function init()
    print("lua init")
end
function cleanup()
    print("lua cleanup")
end
--- @brief 调试时打印变量的值  
--- @param data 要打印的字符串  
--- @param [max_level] table要展开打印的计数，默认nil表示全部展开  
--- @param [prefix] 用于在递归时传递缩进，该参数不供用户使用于  
--- @ref http://dearymz.blog.163.com/blog/static/205657420089251655186/  
function var_dump(data, max_level, prefix)  
    if type(prefix) ~= "string" then  
        prefix = ""  
    end  
    if type(data) ~= "table" then  
        print(prefix .. tostring(data))  
    else  
        print(data)  
        if max_level ~= 0 then  
            local prefix_next = prefix .. "    "  
            print(prefix .. "{")  
            for k,v in pairs(data) do  
                io.stdout:write(prefix_next .. k .. " = ")  
                if type(v) ~= "table" or (type(max_level) == "number" and max_level <= 1) then  
                    print(v)  
                else  
                    if max_level == nil then  
                        var_dump(v, nil, prefix_next)  
                    else  
                        var_dump(v, max_level - 1, prefix_next)  
                    end  
                end  
            end  
            print(prefix .. "}")  
        end  
    end  
end  
  
function vd(data, max_level)  
    var_dump(data, max_level or 5)  
end  

function onSessionReq(sessionid, cmd, body)
    print('onSessionReq', sessionid, cmd, body)
    ip = ''
    ip = h2ext.call('getSessionIp', sessionid)
    
    -- h2ext.gateBroadcastMsg(cmd, string.format('服务器收到消息，sessionid:%d,ip:%s,cmd:%d,data:%s', sessionid, ip, cmd, body))
    h2ext.call('gateBroadcastMsg',cmd, string.format('服务器收到消息，sessionid:%d,ip:%s,cmd:%d,data:%s', sessionid, ip, cmd, body))
    return
end
function onSessionOffline(sessionid)
    print('onSessionOffline', sessionid)
end
function onWorkerCall(cmd, body)
    print('onWorkerCall', cmd, body)
    return "ohok"
end
function testRpc()
    function rpc_cb(ret)
        print('rpc_cb', ret)
    end
    h2ext.workerRPC(0, 101, "hello", rpc_cb)
end
-- testRpc()
-- function timerTest()
    -- function timer_cb()
        -- print('timer_cb')
        -- return
    -- end
    -- h2ext.regTimer(1000, timerTest)
-- end
function timerSync()
    print('timerSync ...............')
end

-- h2ext.regTimer(1000, timerSync)
-- h2ext.regTimer(2000, timerSync)
-- h2ext.regTimer(3000, timerSync)
function dbTest()
    sql = 'create table IF NOT EXISTS foo (num integer);';
    h2ext.query(sql)
    sql = "insert into foo (num) values ('100');";
    ret = h2ext.query(sql)
    print('dbTest', ret)
    ret = h2ext.query('select * from foo limit 1')       -- sync
    print('dbTest', ret)
    function dbTestCb(ret)
        print('dbTest asyncQuery')
        var_dump(ret)
    end
    h2ext.asyncQuery(0, 'select * from foo limit 1', dbTestCb)   -- async
    dbname = 'myDB'
    h2ext.connectDB("sqlite://./test.db", dbname)
    ret = h2ext.queryByName(dbname, 'select * from foo limit 1')
    print('dbTest queryByName')
    var_dump(ret)
    h2ext.asyncQueryByName(dbname, 'select * from foo limit 2', dbTestCb)
end
--dbTest()
print("main.lua................................")

function httpcb(retdata)
    print('httpcb', retdata)
end

-- h2ext.sessionMulticastMsg({1,2,3}, 101, 'ok')
-- ret = h2ext.call("Server.foo", 1, 2.2, 3, 'ddd', {1,2,3,'fff'}, {['a']='b', [111]=222})
-- print(ret)
-- vd(ret)
function testScriptCall(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
    print('testScriptCall', arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
    return 1122334
end

-- h2ext.call("Cache.set", "m.n[10]", "mmm1")
-- cacheRet = h2ext.call("Cache.get", "m.n[10]")
-- print("cacheRet", cacheRet)
-- cacheRet = h2ext.call("Cache.get", "")
-- var_dump(cacheRet)
-- print("cacheRet", h2ext.call("Cache.size", "m"), h2ext.call("Cache.size", "m.n"))
-- print("escape", h2ext.call("escape", "haha\"ok"))
function sayhi(arg)
    print('sayhi *************')
end
