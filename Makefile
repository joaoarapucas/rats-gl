BUILDDIR := build

CC         := gcc
CXX        := g++
PKG_CONFIG ?= pkg-config

# ---------------------------------------------------------------------------
# Plataforma
#   Windows: MSYS2 (ambiente UCRT64 ou MINGW64), executado a partir do shell
#            do MSYS2. Gera um executável estático, sem dependência de DLLs.
#   Linux:   GCC + GLFW do sistema.
#
# A plataforma é detectada pelo alvo do compilador (ex.: x86_64-w64-mingw32),
# e não pela variável de ambiente OS, que nem sempre é repassada ao shell.
# ---------------------------------------------------------------------------
CC_TARGET := $(shell $(CC) -dumpmachine)

ifneq ($(findstring mingw,$(CC_TARGET)),)
  TARGET        := rats.exe
  PKG_FLAGS     := --static
  GLFW_FALLBACK := -lglfw3
  PLATFORM_LIBS := -lopengl32 -lgdi32
  LDFLAGS       += -static
  SANITIZERS    :=
else
  TARGET        := rats
  PKG_FLAGS     :=
  GLFW_FALLBACK := -lglfw
  PLATFORM_LIBS := -ldl
  SANITIZERS    := -fsanitize=address,undefined
endif

GLFW_CFLAGS := $(shell $(PKG_CONFIG) --cflags glfw3 2>/dev/null)
GLFW_LIBS   := $(shell $(PKG_CONFIG) $(PKG_FLAGS) --libs glfw3 2>/dev/null)
ifeq ($(strip $(GLFW_LIBS)),)
  GLFW_LIBS := $(GLFW_FALLBACK)
endif

# ---------------------------------------------------------------------------
# amazing (gerador de labirintos), incluído como git submodule.
#   Clonar:     git clone --recurse-submodules ...
#   Atualizar:  git submodule update --remote external/amazing
# ---------------------------------------------------------------------------
AMAZING_DIR := external/amazing
AMAZING_LIB := $(AMAZING_DIR)/build/libamazing.a

ifeq ($(wildcard $(AMAZING_DIR)/Makefile),)
  $(error Submodule $(AMAZING_DIR) ausente. Execute: git submodule update --init)
endif

CPPFLAGS := -Iinclude -Isrc -I$(AMAZING_DIR)/include $(GLFW_CFLAGS) -MMD -MP
CFLAGS   := -O2
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDLIBS   := $(GLFW_LIBS) $(PLATFORM_LIBS)

CXX_SRCS := $(wildcard src/*.cpp)
C_SRCS   := $(wildcard src/*.c)
OBJS     := $(CXX_SRCS:src/%.cpp=$(BUILDDIR)/%.o) $(C_SRCS:src/%.c=$(BUILDDIR)/%.o)
DEPS     := $(OBJS:.o=.d)

# Mesma extensão do executável principal (.exe no Windows)
TEST_TARGET := $(BUILDDIR)/maze_geometry_test$(suffix $(TARGET))

.PHONY: all run debug test clean FORCE

all: $(TARGET)

$(TARGET): $(OBJS) $(AMAZING_LIB)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Sempre delega ao Makefile do amazing, que recompila apenas o necessário.
# O mesmo compilador e flags são repassados (inclui sanitizers no debug).
$(AMAZING_LIB): FORCE
	$(MAKE) -C $(AMAZING_DIR) lib CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)"

FORCE:

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

# Sanitizers não são suportados pelo GCC do MinGW; no Windows o build de
# debug usa apenas -O0 -g.
debug: CXXFLAGS := $(filter-out -O2,$(CXXFLAGS)) -O0 -g $(SANITIZERS)
debug: CFLAGS   := -O0 -g
debug: LDFLAGS  += $(SANITIZERS)
debug: clean $(TARGET)

# Testes que não dependem de contexto OpenGL
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): tests/MazeGeometryTest.cpp $(BUILDDIR)/MazeGeometry.o $(AMAZING_LIB) | $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

# Limpa também o amazing: objetos compilados com flags diferentes (ex.: com e
# sem sanitizers) não podem ser misturados no link.
clean:
	rm -rf $(BUILDDIR) rats rats.exe
	$(MAKE) -C $(AMAZING_DIR) clean

-include $(DEPS)
