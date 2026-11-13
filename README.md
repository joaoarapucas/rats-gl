# rats-gl
gambling game made in OpenGL and C++

## Clonando

O gerador de labirintos ([amazing](https://github.com/joaoarapucas/amazing)) é um
git submodule em `external/amazing`:

```bash
git clone --recurse-submodules git@github.com:joaoarapucas/rats-gl.git
# se já clonou sem os submodules:
git submodule update --init
```

Para atualizar o amazing para a versão mais recente do repositório dele:

```bash
git submodule update --remote external/amazing
make test
git add external/amazing && git commit -m "chore: bump amazing"
```

O `make` do rats compila o amazing automaticamente (`external/amazing/build/libamazing.a`).

## Build

### Windows (MSYS2)

1. Instale o [MSYS2](https://www.msys2.org/) (ou `winget install MSYS2.MSYS2`).
2. Abra o terminal **MSYS2 UCRT64** e instale as dependências:

   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-pkgconf make
   ```

3. No mesmo terminal, a partir da raiz do projeto:

   ```bash
   make        # gera rats.exe (estático, não depende de DLLs do MinGW)
   make run    # compila e executa
   make debug  # build com -O0 -g
   make clean
   ```

O `make` precisa ser executado a partir do shell do MSYS2: o Makefile usa
comandos POSIX (`mkdir -p`, `rm -rf`) e não funciona com o `cmd.exe` nem com o
GnuWin32 make.

### Linux

Requer `g++`, `make`, `pkg-config` e a GLFW 3 (ex.: `libglfw3-dev` no Debian/Ubuntu).

```bash
make
make run
make debug  # inclui AddressSanitizer e UBSan
```

O executável deve ser iniciado a partir da raiz do projeto, pois os caminhos de
`shaders/` e `textures/` são relativos a ela.
