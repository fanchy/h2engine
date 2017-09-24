function init(){
    h2ext.print("js init ok");
}
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

dbid = h2ext.connectDB("sqlite://./test.db", 'TestGroup');
sql = 'create table  IF NOT EXISTS foo (num integer);';
sql = "insert into foo (num) values ('棒！');";
sql = 'select * from foo';
function dbcb(retdata){
    
    h2ext.print('asyncQuery callback', retdata['datas'], retdata['affectedRows'], retdata['errinfo']);
    //h2ext.asyncQuery(dbid, sql, dbcb);
}
h2ext.asyncQuery(dbid, sql, dbcb);


var retdata = h2ext.query(dbid, sql);
dbcb(retdata)


h2ext.asyncHttp("https://git.oschina.net/ownit/spython/raw/master/ma.py", 1, function(ret){
    h2ext.print("http", ret);
});

// var ret = h2ext.require("foo.js");
// h2ext.print(ret);
h2ext.print("ls", h2ext.ls("./h2js"));

function onWorkerCall(cmd, data){
    h2ext.print('onWorkerCall', cmd, data);
    return 'gotit';
}

h2ext.workerRPC(0, 101, 'OhNice', function(rpcret){
        h2ext.print('rpcret', rpcret);
});

// var ret = h2ext.callFunc("Server.foo", 1, 2.2, "s3");
// h2ext.print(ret);

function testScriptCall(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9){
    h2ext.print('testScriptCall', arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    return 1122334;
}