# Lv2

调用示例代码可以重新从文本中解析出 AST 树，我们需要根据 AST 树生成 Koopa IR 程序。

分多个读写流来管理，而不是多次使用 `freopen`。

注意 `ret` 的返回值需要再使用

```cpp
ret.value->kind.data.integer;
```

来得到。
