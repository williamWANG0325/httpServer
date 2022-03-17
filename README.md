# httpServer

## 主要内容

​		本项目主要通过c++11编写了一个高并发http服务器，主要参考书籍为《Liunx高性能服务器编程》，编写代码的大体思想基本都参照该书，但具体实现方式有很大不同。主要包括以下模块：

-  `log_system`：单例模式与阻塞队列实现异步日志系统，记录服务器状态

-  `sever`：采用 IO 多路复用技术 Epoll 以及高并发 Reactor 模型编写服务器主体逻辑

-  `thread_pool`：采用线程池加快业务处理速度

-  `http_connection`：利用状态机对 HTTP 请求报文进行解析，根据消息码编写相关业务逻辑，支持解析 GET 和 POST 请求

-  `sql_pool`：采用 RAII 机制编写数据库连接池，减少数据库连接建立与关闭的开销，同时实现数据库连接数的自动数量管理

-  `timer`：基于小根堆实现的定时器，用于关闭超时的非活动连接

  我在编写以上模块过程中，尽量减少模块之间的依赖，使得每个模块都可通过独立的测试，同时也保证了复用性。

## 体会与总结

- 事实上，在项目编写的过程中，我对于每个模块的设计都经过认真的思考。在实践过程中也总结出一些经验和技巧，我认为这才是我从该项目中学习到的最重要的东西，我将其简要记录了下来：
  - [线程池](https://github.com/williamWANG0325/httpServer/blob/main/thread_pool/%E7%BA%BF%E7%A8%8B%E6%B1%A0.md)
  - [数据库连接池](https://github.com/williamWANG0325/httpServer/blob/main/sql_pool/%E6%95%B0%E6%8D%AE%E5%BA%93%E8%BF%9E%E6%8E%A5%E6%B1%A0.md)
  - [阻塞队列](https://github.com/williamWANG0325/httpServer/blob/main/log_system/%E9%98%BB%E5%A1%9E%E9%98%9F%E5%88%97.md)
  - [日志系统](https://github.com/williamWANG0325/httpServer/blob/main/log_system/%E6%97%A5%E5%BF%97%E7%B3%BB%E7%BB%9F.md)
  - ...(还有一些没有完善好)
