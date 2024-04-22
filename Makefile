include .env

# Variables
CDEFS += -DFEITCSI_VERSION="\"${FEITCSI_VERSION}"\"

# Target
BIN_DIR = bin
BIN = $(BIN_DIR)/app

# Match all sources in src direcotry
SRCS_DIR = src
SRCS = $(wildcard $(SRCS_DIR)/**/*.cpp $(SRCS_DIR)/*.cpp)

# Builds object list from sources, substitues .cpp for .o
OBJS_DIR = obj
OBJS = $(patsubst $(SRCS_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(SRCS))

# Include headers files
INCLUDE_DIRS = include include/gui lib/include
INCLUDE = $(foreach includedir,$(INCLUDE_DIRS),-I$(includedir))

# Search libraries in following directories
INCLUDE_LIB_DIRS = 
INCLUDE_LIB = $(foreach includedir,$(INCLUDE_LIB_DIRS),-L$(includedir))

# Set compiler, preprocesor and linker flags
CXXFLAGS +=  -g -O1 -Wall -std=c++17 $(CDEFS) $(INCLUDE)
CPPFLAGS += `pkg-config --cflags gtkmm-3.0 libnl-3.0 libnl-genl-3.0 libpcap`
LDFLAGS += $(INCLUDE_LIB)
LDLIBS += `pkg-config --libs gtkmm-3.0 libnl-3.0 libnl-genl-3.0 libpcap`

# Set other tools
MKDIR = mkdir -p

# Avoid filename conflicts
.PHONY: all clean

# Rules
all: $(BIN)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	@$(MKDIR) $(dir $@)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

install:
	cp $(BIN) /usr/local/bin/feitcsi

uninstall:
	rm /usr/local/bin/feitcsi

clean:
	@$(RM) $(BIN)
	@$(RM) $(OBJS)
