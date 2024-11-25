# Level 1

```bash
git diff 501f60b bc7d5f2
```


在 Level 1 中，我们主要要实现一下从原始的 `cpp` 文件编译得到 AST 的过程，在后续的 Level 中，得到的 AST 会被后续用于生成 IR，进而编译得到汇编代码。

相较于文档，唯一需要修改的就是 `sysy.l` 文件，在 LineComment 后追加一个对于 BlockComment 的正则表达式：

```
LineComment   "//".*
BlockComment  \/\*([^\*]*|[\*]+[^\*\/])*[\*]+\/
```

关于正则表达式，我推荐一个可视化网址：[regex-vis](https://regex-vis.com/)。
