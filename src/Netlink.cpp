/*
 * FeitCSI is the tool for extracting CSI information from supported intel NICs.
 * Copyright (C) 2023-2025 Miroslav Hutar.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Netlink.h"

#include <errno.h>
#include <iostream>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/attr.h>

void Netlink::init()
{
        /* int err =  */ this->nlInit(&this->nlstate);
    // TODO if error
}

int Netlink::nlInit(struct nl80211_state *state)
{
    int err;
    std::string errMsg;

    state->nl_sock = nl_socket_alloc();
    if (!state->nl_sock)
    {
        throw std::ios_base::failure("Failed to allocate netlink socket.\n");
        return -ENOMEM;
    }

    if (genl_connect(state->nl_sock))
    {
        errMsg = "Failed to connect to generic netlink.\n";
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);

    /* try to set NETLINK_EXT_ACK to 1, ignoring errors */
    err = 1;
    setsockopt(nl_socket_get_fd(state->nl_sock), SOL_NETLINK,
               NETLINK_EXT_ACK, &err, sizeof(err));

    state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");

    if (state->nl80211_id < 0)
    {
        errMsg = "nl80211 not found.\n";
        err = -ENOENT;
        goto out_handle_destroy;
    }

    return 0;

out_handle_destroy:
    nl_socket_free(state->nl_sock);
    if (!errMsg.empty())
    {
        throw std::ios_base::failure(errMsg);
    }
    
    return err;
}

int Netlink::nlExecCommand(Cmd &cmd)
{
    int err = 0;
    std::string errMsg;
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg)
    {
        throw std::ios_base::failure("failed to allocate netlink message\n");
        return 2;
    }

    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
    struct nl_cb *s_cb = nl_cb_alloc(NL_CB_DEFAULT);

    if (!cb || !s_cb)
    {
        errMsg = "failed to allocate netlink callbacks\n";
        err = 2;
        goto out;
    }

    genlmsg_put(msg, 0, 0, this->nlstate.nl80211_id, 0, cmd.nlFlags, cmd.id, 0);

    switch (cmd.idby)
    {
    case CIB_PHY:
        NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, cmd.device);
        break;
    case CIB_NETDEV:
        NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, cmd.device);
        break;
    case CIB_WDEV:
        NLA_PUT_U64(msg, NL80211_ATTR_WDEV, cmd.device);
        break;
    default:
        break;
    }

    if (cmd.handler)
    {
        err = cmd.handler(&this->nlstate, msg, cmd.args);
    }

    if (err)
        goto out;

    nl_socket_set_cb(this->nlstate.nl_sock, s_cb);

    err = nl_send_auto_complete(this->nlstate.nl_sock, msg);
    
    if (err < 0)
        goto out;

    err = 1;

    nl_cb_err(cb, NL_CB_CUSTOM, this->error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, this->finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, this->ack_handler, &err);
    nl_cb_set(cb, NL_CB_MSG_IN, NL_CB_CUSTOM, cmd.valid_handler ? cmd.valid_handler : this->nlValidHandler, this);
    while (err > 0)
    {
        nl_recvmsgs(this->nlstate.nl_sock, cb);
    }

    if (err < 0)
    {
        errMsg = "command failed: " + std::string(strerror(-err)) + std::to_string(err) + "\n";
    }
out:
    nl_cb_put(cb);
    nl_cb_put(s_cb);
    nlmsg_free(msg);
    if (!errMsg.empty())
    {
        throw std::ios_base::failure(errMsg);
    }
    return err;
nla_put_failure:
    throw std::ios_base::failure("building message failed\n");
    return 2;
}

int Netlink::nlValidHandler(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}

int Netlink::error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
    struct nlmsghdr *nlh = (struct nlmsghdr *)err - 1;
    int len = nlh->nlmsg_len;
    struct nlattr *attrs;
    struct nlattr *tb[NLMSGERR_ATTR_MAX + 1];
    int *ret = (int *)arg;
    int ack_len = sizeof(*nlh) + sizeof(int) + sizeof(*nlh);

    if (err->error > 0)
    {
        /*
         * This is illegal, per netlink(7), but not impossible (think
         * "vendor commands"). Callers really expect negative error
         * codes, so make that happen.
         */

        throw std::ios_base::failure("ERROR: received positive netlink error code " + std::to_string(err->error) + "\n");
        *ret = -EPROTO;
    }
    else
    {
        *ret = err->error;
    }

    if (!(nlh->nlmsg_flags & NLM_F_ACK_TLVS))
        return NL_STOP;

    if (!(nlh->nlmsg_flags & NLM_F_CAPPED))
        ack_len += err->msg.nlmsg_len - sizeof(*nlh);

    if (len <= ack_len)
        return NL_STOP;

    attrs = (nlattr *)((unsigned char *)nlh + ack_len);
    len -= ack_len;

    nla_parse(tb, NLMSGERR_ATTR_MAX, attrs, len, NULL);
    if (tb[NLMSGERR_ATTR_MSG])
    {
        len = strnlen((char *)nla_data(tb[NLMSGERR_ATTR_MSG]),
                      nla_len(tb[NLMSGERR_ATTR_MSG]));

        throw std::ios_base::failure("kernel reports: " + std::string((char *)nla_data(tb[NLMSGERR_ATTR_MSG])) + "\n");
    }

    return NL_STOP;
}

int Netlink::finish_handler(struct nl_msg *msg, void *arg)
{
    int *ret = (int *)arg;
    *ret = 0;
    return NL_SKIP;
}

int Netlink::ack_handler(struct nl_msg *msg, void *arg)
{
    int *ret = (int *)arg;
    *ret = 0;
    return NL_STOP;
}

/* 1. EPERM: Operation not permitted - The requested operation is not allowed due to insufficient permissions.
2. ESRCH: No such process - The specified process or resource could not be found.
3. EINTR: Interrupted system call - The system call was interrupted by a signal.
4. EBADF: Bad file descriptor - The file descriptor provided is not valid.
5. EFAULT: Bad address - The provided memory address is invalid or inaccessible.
6. EINVAL: Invalid argument - One or more of the arguments provided is not valid.
7. EIO: Input/output error - An error occurred during input or output operations.
8. ENOMEM: Out of memory - Insufficient memory is available to complete the requested operation.
9. EACCES: Permission denied - The requested operation is not allowed due to permission restrictions.
10. EBUSY: Device or resource busy - The requested device or resource is currently in use.

1. EPERM: -1
2. ESRCH: -3
3. EINTR: -4
4. EBADF: -9
5. EFAULT: -14
6. EINVAL: -22
7. EIO: -5
8. ENOMEM: -12
9. EACCES: -13
10. EBUSY: -16 */