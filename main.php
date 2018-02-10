<?
function init(){
    print("php init...\n");
}
function cleanup(){
    print("php cleanup...\n");
}

// we set a larger memory limit just in case
ini_set('memory_limit', '1000M');


function onSessionReq($sessionid, $cmd, $body){
    print('onSessionReq:'.$sessionid.','.$cmd.','.$body."\n");
    $ip = h2ext::getSessionIp($sessionid);
    h2ext::sessionSendMsg($sessionid, $cmd, '服务器收到消息，sessionid:'.$sessionid.',ip:'.$ip.',cmd:'.$cmd.',data:'. $body);
    return;
}
function onSessionOffline($sessionid){
    print('onSessionOffline:'.$sessionid."\n");
}

function timer_cb_p(){
    print("timer_cb_p:\n");
    print("timer_cb_p: end***************************\n");
}
function timer_cb(){
    h2ext::regTimer(1, "timer_cb");
}
for ($x=0; $x<=1000; $x++) {
    //h2ext::regTimer(1, timer_cb);
}
h2ext::regTimer(1000, timer_cb_p);
h2ext::regTimer(2000, timer_cb_p);
h2ext::regTimer(3000, timer_cb_p);
print ("main.php.........\n");
$dbid = h2ext::connectDB("sqlite://./test.db", 'TestGroup');
$sql = 'select * from foo';
function db_cb($retdata){
    //var_dump($retdata);
    
    h2ext::asyncQuery($dbid, $sql, db_cb);
}
//h2ext::asyncQuery($dbid, $sql, db_cb);
for ($x=0; $x<=1000000; $x++) {
    //h2ext::query($dbid, $sql);
}
function onWorkerCall($cmd, $data){
    //print('onWorkerCall'.$cmd.":".$data);
    return "rpcok!!";
}

$url = "https://git.oschina.net/ownit/spython/raw/master/ma.py";

function http_cb($rpcret){
    //print('rpcret:'.$rpcret);
    
    h2ext::asyncHttp($url, 1, http_cb);
}
//h2ext::asyncHttp($url, 1, http_cb);


function rpc_cb($rpcret){
    print('rpcret:'.$rpcret);

    // 
    // h2ext::workerRPC(0, 101, 'OhNice', rpc_cb);
}
h2ext::workerRPC(0, 101, 'OhNice', rpc_cb);
// for($i = 0; $i < 10000*1000; $i++)
    // $ret = h2ext::callFunc("Server.foo", 12345678901234567, 'ssss', 22.33, array('cc' => 'b'), array(1,2,3));
// var_dump($ret);

function testScriptCall($arg1 = 0, $arg2 = 0, $arg3 = 0, $arg4 = 0, $arg5 = 0, $arg6 = 0, $arg7 = 0, $arg8 = 0, $arg9 = 0){
    print('testScriptCall'.":".$arg1.":".$arg2.":".$arg3.":".$arg4.":".$arg5.":".$arg6.":".$arg7.":".$arg8.":".$arg9."\n");
    return 1122334;
}
function testScriptCache(){
    print("xxx\n");
    h2ext::callFunc("Cache.set", "m.n[10]", "mmm1");

    $cacheRet = h2ext::callFunc("Cache.get", "m.n[0]");
    print("cacheRet".$cacheRet);
    $cacheRet = h2ext::callFunc("Cache.get", "");
    //var_dump($cacheRet);
    print("cacheRet:".h2ext::callFunc("Cache.size", "m").":".h2ext::callFunc("Cache.size", "m.n")."\n");
}

testScriptCache();

function dbTestCb($ret){
    var_dump('dbTest asyncQuery');
    //var_dump(ret)
}
function dbTest(){
    
    $sql = 'create table IF NOT EXISTS foo (num integer);';
    h2ext::query($sql);
    
    $sql = "insert into foo (num) values ('100');";
    $ret = h2ext::query($sql);
    
    var_dump('dbTest');
    
    $ret = h2ext::query('select * from foo limit 1')  ;     
    //h2ext::print('dbTest', ret);
    
    h2ext::asyncQuery(0, 'select * from foo limit 1', dbTestCb);
    $dbname = 'myDB';
    h2ext::connectDB("sqlite://./test.db", $dbname);
    $ret = h2ext::queryByName($dbname, 'select * from foo limit 1');
    //h2ext::print('dbTest queryByName')
    //var_dump(ret)
    h2ext::asyncQueryByName(dbname, 'select * from foo limit 2', dbTestCb);
}
dbTest();
?>
