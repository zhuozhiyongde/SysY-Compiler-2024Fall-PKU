# Level 0

## Docker

Docker 部分建议找一个成熟一点的教程。我已经之前用过很多次了，过。

我选择维护一个 `docker-compose.yml` 文件：

```yaml
services:
  compiler:
    image: maxxing/compiler-dev
    container_name: compiler
    volumes:
      - ./:/root/compiler
    command: bash -c "sleep infinity"
    network_mode: host
```

这个文件可以将当前目录直接映射到 Docker 容器中，这样内外的修改都可以实时反映到另一边。

想要进入容器，可以输入：

```bash
docker exec -it compiler bash -c "cd compiler; bash"
```

## Tmux

接着，我使用 tmux 来管理内外终端环境：

```bash
tmux new -s compiler
docker compose exec -it compiler bash
cd ~/compiler
```

输入 <kbd>Ctrl-b</kbd>，然后按 d 可以退出 tmux 会话，再次输入 `tmux attach -t compiler` 可以重新进入。

你也可以将以下两行追加到 `~/.bashrc` 或 `~/.zshrc` 文件中，节省输入：

```bash
alias tn="tmux new -s"
alias ta="tmux attach -t"
```

这样，以后我只需要输入 `ta compiler` 就可以进入 tmux 会话了。

## 项目结构

```
.
├── docker-compose.yml
├── hello.cpp
├── hello.koopa
├── ir.koopa
├── Makefile
└── src
    ├── include
    │   └── koopa.h
    ├── main.cpp
    ├── sysy.l
    └── sysy.y
```

如你所见，我们重点关注 `src` 目录下的文件：

`includes/` 目录下主要是一些头文件，对于每个模块，我们将头文件 hpp 放在这里，将实现的 cpp 文件放在 `src/` 目录下。

其中，`koopa.h` 是 Koopa IR 的声明文件，我们将它放在这里主要是为了避免 IDE 引用报错，你可以在 [这里](https://github.com/pku-minic/koopa/blob/master/crates/libkoopa/include/koopa.h) 找到它的原始文件。

其他都是我们自己实现的文件：

- `sysy.l` 是词法分析器的配置文件，会被 lex 工具使用，进一步生成所需词法分析器，并在 `Makefile` 中自动完成编译、链接。
- `sysy.y` 是语法分析器的配置文件，会被 bison 工具使用，进一步生成所需语法分析器，并在 `Makefile` 中自动完成编译、链接。
- `main.cpp` 是主程序，负责调用各个模块。

当我们按照文档，复制好了 `sysy.l` 和 `sysy.y` 文件后，在 Docker 容器内运行 `make` 指令，即可得到我们的可执行文件 `build/compiler`，用于后续测试。

## 测试

各个本地测试点的输入可以在 `/opt/bin/testcases/` 下找到。