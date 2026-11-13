TARGET   := rats
BUILDDIR := build

CC       := gcc
CXX      := g++

CPPFLAGS := -Iinclude -Isrc $(shell pkg-config --cflags glfw3) -MMD -MP
CFLAGS   := -O2
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDLIBS   := $(shell pkg-config --libs glfw3) -ldl

CXX_SRCS := $(wildcard src/*.cpp)
C_SRCS   := $(wildcard src/*.c)
OBJS     := $(CXX_SRCS:src/%.cpp=$(BUILDDIR)/%.o) $(C_SRCS:src/%.c=$(BUILDDIR)/%.o)
DEPS     := $(OBJS:.o=.d)

.PHONY: all run debug clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILDDIR)/%.o: src/%.cpp | $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: src/%.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Código de terceiros: sem warnings
$(BUILDDIR)/stb_image.o: CXXFLAGS += -w

$(BUILDDIR):
	mkdir -p $@

# Os caminhos de shaders/texturas são relativos à raiz do projeto
run: $(TARGET)
	./$(TARGET)

debug: CXXFLAGS := $(filter-out -O2,$(CXXFLAGS)) -O0 -g -fsanitize=address,undefined
debug: CFLAGS   := -O0 -g
debug: LDFLAGS  += -fsanitize=address,undefined
debug: clean $(TARGET)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

-include $(DEPS)
