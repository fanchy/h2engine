# -*- coding: utf-8 -*-

import h2ext

def init():
    print('py init.....')
def cleanup():
    print('py cleanup.....')

def onSessionReq(sessionid, cmd, body):
    print('onSessionReq', sessionid, cmd, body)
    ip = h2ext.getSessionIp(sessionid)
    h2ext.gateBroadcastMsg(cmd, '服务器收到消息，sessionid:%d,ip:%s,cmd:%d,data:%s'%(sessionid, ip, cmd, body))
    return
def onSessionOffline(sessionid):
    print('onSessionOffline', sessionid)
    return

def sharedMemTest():
    d = h2ext.get_shared_data(0, sessionid)
    print('shared_data', sessionid, d.strData)
def onWorkerCall(cmd, body):
    print('onWorkerCall', cmd, body)
    return "ohok"

def rpc_cb(ret):
    print('rpc_cb', ret)
#h2ext.workerRPC(0, 101, "hello", rpc_cb)


def timerTest():
    def timer_cb():
        print('timer_cb')
        return
    h2ext.regTimer(1000, timer_cb)
def dbTest():
    sql = 'create table IF NOT EXISTS foo (num integer);';
    h2ext.query(sql)
    sql = "insert into foo (num) values ('100');";
    ret = h2ext.query(sql)
    print('dbTest', ret)
    ret = h2ext.query('select * from foo limit 5')       #sync
    print('dbTest', ret)
    def dbTestCb(ret = None):
        print('dbTest asyncQuery', ret)
    h2ext.asyncQuery(0, 'select * from foo limit 5', dbTestCb)   #async
    dbname = 'myDB'
    h2ext.connectDB("sqlite://./test.db", dbname)
    ret = h2ext.queryByName(dbname, 'select * from foo limit 5')
    print('dbTest queryByName', ret)
    h2ext.asyncQueryByName(dbname, 'select * from foo limit 5', dbTestCb)
#dbTest()
print('main.py'+'*'*10)
def httpcb(retdata):
    print('httpcb', retdata)
#h2ext.asyncHttp("http://www.baidu.com", 1, httpcb)
def onSyncSharedData(cmd, data):
    print('onSyncSharedData', cmd, data)
  
#ret = h2ext.callFunc("Entity.totalNum")
#print('ret', type(ret), ret)
def testScriptCall(arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0, arg5 = 0, arg6 = 0, arg7 = 0, arg8 = 0, arg9 = 0):
    print('testScriptCall', arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
    return 1122334
h2ext.callFunc("Cache.set", "b", "bbbb")
h2ext.callFunc("Cache.set", "m.n[10]", "mmm1")
cacheRet = h2ext.callFunc("Cache.get", "m.n[0]")
print("cacheRet", cacheRet)
multigetRet = h2ext.callFunc("Cache.multiget", ["m.n", "b"])
print("multigetRet", multigetRet)
cacheRet = h2ext.callFunc("Cache.get", "")
print("cacheRet", cacheRet, h2ext.callFunc("Cache.size", "m"), h2ext.callFunc("Cache.size", "m.n"))

def test(arg = None, arg2 = None):
    print('test func', arg, arg2)
    return arg

def testTask(entity):
    h2ext.callFunc("Task.addTaskCfg", {"cfgid":1, "taskLine":2, "nextTask":2, "finishConditionType":"killMonster", "finishConditionObject":"cat", "finishConditionValue":1})
    h2ext.callFunc("Task.addTaskCfg", {"cfgid":2, "taskLine":2, "nextTask":0, "finishConditionType":"killMonster", "finishConditionObject":"dog", "finishConditionValue":1})
    taskCfg = h2ext.callFunc("Task.getTaskCfg", 1)
    print("taskCfg", taskCfg)
    h2ext.callFunc("Task.genTask", entity, 1)
    h2ext.callFunc("Task.acceptTask", entity, 1)
    h2ext.callFunc("Task.triggerEvent", entity, 'killMonster', 'cat', 1)
    h2ext.callFunc("Task.endTask", entity, 1)
    allTask = h2ext.callFunc("Task.getTask", entity, 2)
    print("testTask allTask", allTask)
    return
    
def sayhi(arg = None):
    print('sayhi *************')