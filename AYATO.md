

```c++
std::set<std::string> dirList = ft::CreateDirectoryList("~");
  for (std::set<std::string>::iterator it = dirList.begin(); it != dirList.end(); it++) {
    body << "\t\t" << *it << "<br>" << CRLF;
  }
```

do_get()した際によびだせばいい。
条件は以下の感じ
```
if (ret_val < 0) { if autoindex==true - body = render_html() else return 404 ?? 
```

`server.cpp`でなんとか条件作っておいたら良さそうだねー
