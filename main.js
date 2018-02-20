
function cleanup(){
    h2ext.print("js cleanup ok");
}
function onSessionReq(sessionid, cmd, body){
    h2ext.print('onSessionReq', sessionid, cmd, body);
    ip = h2ext.getSessionIp(sessionid);
    h2ext.gateBroadcastMsg(cmd, 
        '服务器收到消息，sessionid:'+sessionid+',ip:'+ip+',cmd:'+cmd+',data:'+ body);
    return;
}
function onSessionOffline(sessionid){
    h2ext.print('onSessionOffline', sessionid);
}
// h2ext.sessionSendMsg(100000000000, 2, 'ccccccccc');
// h2ext.sessionMulticastMsg([1,2,3], 101, 'dddd');
// h2ext.sessionClose(33);
// h2ext.sessionChangeWorker(4, 1, 'extraAA');
// gate = h2ext.getSessionGate(55);
// h2ext.print(gate);
function timercb(){
    h2ext.regTimer(1, timercb);
}

function dbTest(){
    
    sql = 'create table IF NOT EXISTS foo (num integer);';
    h2ext.query(sql);
    
    sql = "insert into foo (num) values ('100');";
    ret = h2ext.query(sql);
    
    h2ext.print('dbTest');
    
    ret = h2ext.query('select * from foo limit 1')  ;     
    h2ext.print('dbTest query', ret);
    function dbTestCb(ret){
        h2ext.print('dbTest asyncQuery')
        //var_dump(ret)
    }
    //h2ext.asyncQuery(0, 'select * from foo limit 1', dbTestCb);
    //return;
    dbname = 'myDB'
    h2ext.connectDB("sqlite://./test.db", dbname)
    ret = h2ext.queryByName(dbname, 'select * from foo limit 1')
    h2ext.print('dbTest queryByName')
    //var_dump(ret)
    h2ext.asyncQueryByName(dbname, 'select * from foo limit 2', dbTestCb)
}


// h2ext.asyncHttp("https://git.oschina.net/ownit/spython/raw/master/ma.py", 1, function(ret){
    // h2ext.print("http", ret);
// });

// var ret = h2ext.require("foo.js");
// h2ext.print(ret);
h2ext.print("ls", h2ext.ls("./h2js"));

function onWorkerCall(cmd, data){
    h2ext.print('onWorkerCall', cmd, data);
    return 'gotit';
}

// h2ext.workerRPC(0, 101, 'OhNice', function(rpcret){
        // h2ext.print('rpcret', rpcret);
// });

// var ret = h2ext.callFunc("Server.foo", 1, 2.2, "s3");
// h2ext.print(ret);

function testScriptCall(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9){
    h2ext.print('testScriptCall', arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    return 1122334;
}
function testCode(){
    h2ext.callFunc("Cache.set", "m.n[10]", "mmm1");
    cacheRet = h2ext.callFunc("Cache.get", "m.n[10]");
    h2ext.print("cacheRet", cacheRet);
    cacheRet = h2ext.callFunc("Cache.get", "");
    h2ext.print(cacheRet['m']['n']);
    h2ext.print("cacheRet", h2ext.callFunc("Cache.size", "m"), h2ext.callFunc("Cache.size", "m.n"));
    dbTest();
}
function init(){
    testCode();
    dbTest();
    h2ext.print("js init ok");
}

FooClass ={
    'sayhi' : function(){
        h2ext.print("Hello World S!");
    }
};
