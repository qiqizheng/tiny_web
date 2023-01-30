# tiny_web
csapp tiny web 服务器

启动方式：
```
gcc tiny.c csapp.c -o tiny 
cd cgi-bin
gcc adder.c -o adder -I../
cd ..
./tiny 8888
```

游览器访问
```
ip:8888/cgi-bin/adder?xxxx&yyyy
```
