# H2Engine 服务器引擎
H2服务器引擎架构是轻量级的，与其说是引擎，个人觉得称之为平台更为合适。因为它封装的功能少之又少，但是提供了非常简洁方便的扩展机制，使得可以用C++、python、lua、js、php来开发具体的服务器功能。H2引擎的灵感来源于web服务器Apache。
1.  H2引擎集成了websocket，也推荐大家在长连接应用中，逐渐使用websocket。
2.  协议的封包pb、thrift已经很够用了，H2引擎支持pb、thrift、json以及传统二进制struct，但是推荐thrift，主要是效率和多语言支持都更好。
3.  基于网游服务器的场景，H2引擎考虑到单台物理机的处理能力当前足以应付单服的需求，所以将H2的架构设计为部署在同机上，这样大大简化了服务器的架构，多gate的架构其实来源于rpg刚兴起的年代，那时候服务器的内存有限，cpu多核也还没流行，但是今非昔比，单机模式也就是伪分布式模式其实更符合实际。
4.  针对传统网游服务器架构中多进程数据共享的痛点，H2做了特殊的设计，由于H2Worker在同一台机器上，得以使H2可以通过共享内存共享数据。
　　大家知道，Apache+php之所以在web领域里流行，还有很大一个原因是php的框架又多又好用，相比而言，网游服务器领域的引擎、框架都太落后了，主要原因还是服务器没有形成标准。从web的成熟经验来看，功能开发的快，就要有好多框架，要有好的框架，就要有成熟标准的引擎，现在市面上有些游戏服务器引擎就经常会糅合引擎和框架的功能，有的甚至夹杂了游戏服务器的数据结构和游戏逻辑。H2Engine的设计哲学，引擎的归引擎，框架的归框架，虽然跟Apache相比距离“引擎”的称号相距甚远，但是这是H2的目标。另外，基于H2的框架也会不断的增加完善。举个例子，针对rpg游戏，我们可以设计出一套c++的框架，比如封装地图管理、角色管理、道具管理、任务系统、成就系统、副本系统、npc系统等，想想看，2d rpg领域相关的系统还是很好抽象的。问题是没有标准的、成熟的引擎作为基础。相关从业人员应该有共鸣，比如A团队开发一套任务系统，给B团队也是用不了啊，大家的定时器、数据库接口都不一样，无法做到拿来就用。如果大家都用H2，别人开源的系统分分钟就可以拿来用，想象下还是挺美好的。不同的游戏类型框架实现是不一样的，不同语言实现细节也会不同，使用H2Engine引擎后可以根据不同游戏类型、不同语言分类框架。
## 构建
H2Engine目前只有Linux版本，使用cmake，确保系统安装了cmake
```shell
$ cmake  
```
H2Engine进程分另个，h2engine 和h2worker ,其中h2worker根据使用语言的不同，分h2workerpy、h2workerlua、h2workerjs、h2workerphp,根据你使用的语言构建你需要的h2worker即可。
```shell
$ make h2engine
$ make h2workerpy
$ make h2workerlua
$ make h2workerjs
$ make h2workerphp
```
## 依赖说明：
- cmake:构建的时候需要
- python2.6或python2.7:构建h2workerpy的时候需要
- lua5:构建h2workerlua的时候需要
- js v8:构建h2workerjs的时候需要
- libphp5:构建h2workerphp的时候需要,注意需要下载php源码编译出来允许嵌入的版本，./configure --enable-embed  --prefix=~/php5dir - --with-iconv=/usr/local/libiconv

## Further Reading

- [documentation](http://h2cloud.org)

## Contact Information
- [GitHub](https://github.com/fanchy/h2engine)
- QQ:693654841
- [zxfown@gmail.com](mailto:zxfown@gmail.com)
