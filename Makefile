CXX     ?= g++
SRC      = src
OBJS     = obj

LIB      = lib

THIRDPARTY = thirdparty
INCLUDEDIR = $(THIRDPARTY)/includes

PEEREXEC    = peer
TRACKEREXEC = tracker


WARNINGS   = -Wall -Wextra -pedantic -g
NOWARNINGS = -w
IDIRS      = -I$(SRC) -I$(INCLUDEDIR)
LIBS       = -lrt -pthread -lstdc++fs
LDIRS      = -L$(LIB)

CXXFLAGS     = $(IDIRS) -std=c++17 $(WARNINGS) $(DEBUG)
CXXFASTFLAGS = $(IDIRS) -std=c++17 \
	-Ofast \
	-march=native \
	-ffast-math \
	-funsafe-math-optimizations \
	-fassociative-math \
	-freciprocal-math \
	-ffinite-math-only \
	-fno-signed-zeros \
	-fno-trapping-math \
	-funroll-loops

find = $(shell find $1 -type f ! -path $3 -name $2 -print 2>/dev/null)

TRACKERSRCS := $(call find, $(SRC)/, "*.cpp", "src/peer/*")
PEERSRCS := $(call find, $(SRC)/, "*.cpp", "src/tracker/*")
TRACKEROBJECTS := $(TRACKERSRCS:%.cpp=$(OBJS)/%.o)
PEEROBJECTS := $(PEERSRCS:%.cpp=$(OBJS)/%.o)

CLEAR  = [0m
CYAN   = [1;36m
GREEN  = [1;32m
YELLOW = [1;33m
WHITE  = [1;37m

MAKEFLAGS = -j

xoutofy = $(or $(eval PROCESSED := $(PROCESSED) .),$(info $(WHITE)[$(YELLOW)$(words $(PROCESSED))$(WHITE)] $1$(CLEAR)))

.PHONY: tracker peer git

all: tracker peer

tracker: $(TRACKEROBJECTS)
	@$(call xoutofy,$(GREEN)Linking $(if $(FAST),fast,debug) $(TRACKEREXEC))
	$(CXX) $(if $(FAST),$(CXXFASTFLAGS),$(CXXFLAGS)) $(TRACKEROBJECTS) -o $(TRACKEREXEC) $(LIBS) $(LDIRS)

peer: $(PEEROBJECTS)
	@$(call xoutofy,$(GREEN)Linking $(if $(FAST),fast,debug) $(PEEREXEC))
	$(CXX) $(if $(FAST),$(CXXFASTFLAGS),$(CXXFLAGS)) $(PEEROBJECTS) -o $(PEEREXEC) $(LIBS) $(LDIRS) -pthread

# Compiles regular cpp files
$(OBJS)/%.o: %.cpp
	@$(call xoutofy,$(CYAN)Compiling $<)
	@mkdir -p $(dir $@)
	$(CXX) $(if $(FAST),$(CXXFASTFLAGS),$(CXXFLAGS)) -o $@ -c $<


clean:
	@echo Cleaning...
	@rm -rf $(OBJS) $(TRACKEREXEC) $(PEEREXEC)
	@echo Done!

git:
	git add --all
	git commit
	git push

c: clean

.PHONY: c clean
