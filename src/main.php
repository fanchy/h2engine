<?

// test.php - file for exploring php embed
// Copyright (c) 2007 Andrew Bosworth, Facebook, inc
// All rights reserved
//
// This file is loaded by the PHP interpreter compiled into example

// we set a larger memory limit just in case
ini_set('memory_limit', '1000M');

function h2DefaultErrorHandler($errno, $errstr, $errfile, $errline) {

    global $h2ext;
    $errmsg = "exception:$errfile:$errline:$errstr\n";
    
    //var_dump(debug_backtrace());
    $dbtrace = debug_backtrace();
    for ($i = 0; $i < count($dbtrace); ++$i){
        $rowtrace = $dbtrace[$i];
        $errmsg .= "[".($i+1)."]".$rowtrace['file'].":".$rowtrace['line']." ".$rowtrace['function'];
    }
    $h2ext->log(2, "FFWORKER_PHP", $errmsg);
    exit;
}
// set_error_handler("h2DefaultErrorHandler", E_ALL & ~E_NOTICE);


function timer_cb_p(){
    print("timer_cb_p:\n");
    print("timer_cb_p: end\n");
}
function timer_cb(){
    global $h2ext;
    
    $h2ext->regTimer(1, "timer_cb");
}
for ($x=0; $x<=1000; $x++) {
    //$h2ext->regTimer(1, timer_cb);
}
$h2ext->regTimer(1000, timer_cb_p);
$h2ext->regTimer(2000, timer_cb_p);
h2ext_inst()->regTimer(3000, timer_cb_p);
print ("main.php.........\n");
$dbid = $h2ext->connectDB("sqlite://./test.db", 'TestGroup');
$sql = 'select * from foo';
function db_cb($retdata){
    //var_dump($retdata);
    global $h2ext;
    $h2ext->asyncQuery($dbid, $sql, db_cb);
}
//$h2ext->asyncQuery($dbid, $sql, db_cb);
for ($x=0; $x<=1000000; $x++) {
    //$h2ext->query($dbid, $sql);
}
function onWorkerCall($cmd, $data){
    //print('onWorkerCall'.$cmd.":".$data);
    return "rpcok!!";
}

$url = "https://git.oschina.net/ownit/spython/raw/master/ma.py";

function http_cb($rpcret){
    //print('rpcret:'.$rpcret);
    global $h2ext;
    $h2ext->asyncHttp($url, 1, http_cb);
}
//$h2ext->asyncHttp($url, 1, http_cb);

function cleanup(){
    print("cleanup...\n");
}


function rpc_cb($rpcret){
    print('rpcret:'.$rpcret);

    // global $h2ext;
    // $h2ext->workerRPC(0, 101, 'OhNice', rpc_cb);
}
$h2ext->workerRPC(0, 101, 'OhNice', rpc_cb);
// for($i = 0; $i < 10000*1000; $i++)
    // $ret = $h2ext->callFunc("Server.foo", 12345678901234567, 'ssss', 22.33, array('cc' => 'b'), array(1,2,3));
// var_dump($ret);
?>
