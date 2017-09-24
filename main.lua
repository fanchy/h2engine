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

CMD_LOGIN = 1
CMD_CHAT  = 2
function onSessionReq(sessionid, cmd, body)
    print('onSessionReq', sessionid, cmd, body)
    ip = h2ext.getSessionIp(sessionid)
    h2ext.gateBroadcastMsg(cmd, string.format('服务器收到消息，sessionid:%d,ip:%s,cmd:%d,data:%s', sessionid, ip, cmd, body))
    return
end
function onSessionOffline(sessionid)
    print('onSessionOffline', sessionid)
end
-- class Player:
    -- function __init__(self, sessionid)
        -- self.sessionid = sessionid
        -- self.name = ''
    -- function sendChat(self, data):
        -- return h2ext.sessionSendMsg(self.sessionid, CMD_CHAT, data)
-- class PlayerMgr:
    -- function __init__(self):
        -- self.allSession2Player = {}
    -- function allocPlayer(self, sessionid):
        -- player = self.allSession2Player.get(sessionid)
        -- if not player:
            -- player = Player(sessionid)
            -- self.allSession2Player[sessionid] = player
        -- return player
    -- function delPlayer(self, sessionid):
        -- return self.allSession2Player.pop(sessionid, None)
    -- function findPlayerByName(self, name):
        -- for k, v in self.allSession2Player.iteritems():
            -- if v.name == name:
                -- return v
        -- return None
    -- function foreach(self, func):
        -- for k, v in self.allSession2Player.iteritems():
            -- func(v)

-- objMgr = PlayerMgr()
-- function onSessionReq(sessionid, cmd, body)
    -- print('onSessionReq', sessionid, cmd, body)
    -- player = objMgr.allocPlayer(sessionid)
    -- if cmd == CMD_LOGIN then
        -- name = body
        -- oldPlayer = objMgr.findPlayerByName(name)
        -- if oldPlayer then
            -- h2ext.sessionClose(oldPlayer.sessionid)
            -- objMgr.delPlayer(sessionid)
        -- end
        -- player.name = name
        -- print('player.name', player.name)
        -- allPlayerList = []
        -- function getAllPlayer(eachPlayer)
            -- allPlayerList.append(eachPlayer.name)
        -- end
        -- objMgr.foreach(getAllPlayer)
        -- function notifyOnline(eachPlayer):
            -- eachPlayer.sendChat('[%s] online ip=[%s]'%(name, h2ext.getSessionIp(sessionid)))
            -- eachPlayer.sendChat('current online list:%s'%(str(allPlayerList)))
        -- end
        -- objMgr.foreach(notifyOnline)
    -- else if cmd == CMD_CHAT
        -- msg = '[%s] say:%s'%(player.name, body)
        -- function notifyOnline(eachPlayer)
            -- eachPlayer.sendChat(msg)
        -- end
        -- objMgr.foreach(notifyOnline)
    -- end
-- function onSessionOffline(sessionid):
    -- print('onSessionOffline', sessionid)
    -- oldPlayer = objMgr.delPlayer(sessionid)
    -- if oldPlayer then
        -- allPlayerList = []
        -- function getAllPlayer(eachPlayer)
            -- allPlayerList.append(eachPlayer.name)
        -- end
        -- objMgr.foreach(getAllPlayer)
        -- function notifyOnline(eachPlayer)
            -- eachPlayer.sendChat('%s offline！'%(eachPlayer.name))
            -- eachPlayer.sendChat('current online list:%s'%(str(allPlayerList)))
        -- end
        -- objMgr.foreach(notifyOnline)
    -- end
-- function sharedMemTest()
    -- d = h2ext.get_shared_data(0, sessionid)
    -- print('shared_data', sessionid, d.strData)
-- end
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
testRpc()
-- function timerTest()
    -- function timer_cb()
        -- print('timer_cb')
        -- return
    -- end
    -- h2ext.regTimer(1000, timerTest)
-- end
function timerSync()
    h2ext.writeLockGuard();
    h2ext.syncSharedData(100, "helloworld")
    print('timerSync ...............')
end

h2ext.regTimer(1000, timerSync)
h2ext.regTimer(2000, timerSync)
h2ext.regTimer(3000, timerSync)
-- function dbTest()
    -- dbhost = "mysql://127.0.0.1:3306/root/rootpassword/dbname"
    -- dbid   = h2ext.connectDB(dbhost, 'TestGroup')
    -- function cb(ret)
        -- print('cb', ret)
        -- return
    -- end
    -- h2ext.asyncQuery(dbid, 'select * from user_log limit 1', cb)   #async
    -- ret = h2ext.query(dbid, 'select * from user_log limit 1')       #sync
-- end
-- print('main.py'+'*'*10)

print("main.lua................................")

function httpcb(retdata)
    print('httpcb', retdata)
end
-- h2ext.asyncHttp("http://www.baidu.com", 1, httpcb)
-- ret = h2ext.syncHttp("http://www.baidu.com", 1)
-- httpcb(ret)
h2ext.sessionMulticastMsg({1,2,3}, 101, 'ok')
-- ret = h2ext.callFunc("Server.foo", 1, 2.2, 3, 'ddd', {1,2,3,'fff'}, {['a']='b', [111]=222})
-- print(ret)
-- vd(ret)
function testScriptCall(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
    print('testScriptCall', arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
    return 1122334
end

