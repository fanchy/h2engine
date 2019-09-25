using System;
using System.Net.Sockets;
using System.Collections.Generic;

using pb = global::Google.Protobuf;
namespace ff
{
    public class Role
    {
        public Int64 nSessionID;
        public string strName;
        public int nLevel;
        public int x;
        public int y;
        public int hp;
        public int direction;
        public Role()
        {
            nSessionID = 0;
            strName = "";
            nLevel = 0;
            Random rd = new Random();
            x = 30+ rd.Next() % 5;
            y = 30 + rd.Next() % 5;
            hp = 100;
            direction = 4;
        }
        public Int64 getID()
        {
            return nSessionID;
        }
    };
    public class Player: Role
    {
        public Player(): base()
        {
        }
    };
    public class Monster : Role
    {
        public Monster() : base()
        {
        }
    };
    public delegate void CmdHandler(Player p, string data);
    public delegate void PbHandler<RET_TYPE>(Player p, RET_TYPE t);//泛型委托
    public class FFWorker
    {
        protected long m_nIDGenerator;
        protected int m_nWorkerIndex;//!这是第几个worker，现在只有一个worker
        protected string m_strWorkerName;
        protected EmptyMsgRet RPC_NONE;
        protected string m_strDefaultGate;
        FFRpc m_ffrpc;
        protected Dictionary<Int64, Role> m_dictRoles;
        protected Dictionary<int, CmdHandler> m_dictCmd2Func;
        public FFWorker()
        {
            m_nIDGenerator = 0;
            m_nWorkerIndex = 0;
            m_strWorkerName = "";
            m_strDefaultGate = "gate#0";
            m_ffrpc = null;
            m_dictRoles = new Dictionary<Int64, Role>();
            m_dictCmd2Func = new Dictionary<int, CmdHandler>();
            RPC_NONE = new EmptyMsgRet();

            this.BindHandler<Pbmsg.LoginReq>(Pbmsg.ClientCmdDef.CLogin, this.HandleLogin)
                .BindHandler<Pbmsg.RunReq>(Pbmsg.ClientCmdDef.CRun, this.HandleRun)
                .BindHandler<Pbmsg.AttackReq>(Pbmsg.ClientCmdDef.CAttack, this.HandleAttack)
                ;
            int nGenId = 80;
            nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物1", x = 29, y = 29 };
            nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物2", x = 42, y = 27 };
            nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物3", x = 48, y = 19 };
            nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物4", x = 52, y = 31 };
            nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物5", x = 45, y = 45 };
            nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物6", x = 51, y = 60 };
        }
        public bool Open(string strBrokerHost, int nWorkerIndex)
        {
            m_nWorkerIndex = nWorkerIndex;
            m_strWorkerName = string.Format("worker#{0}", m_nWorkerIndex);
            m_ffrpc = new FFRpc(m_strWorkerName);
            if (m_ffrpc.Open(strBrokerHost) == false)
            {
                FFLog.Error("worker ffrpc open failed!");
                return false;
            }

            m_ffrpc.Reg<RouteLogicMsgReq, EmptyMsgRet>(this.OnRouteLogicMsgReq);
            m_ffrpc.Reg<SessionOfflineReq, EmptyMsgRet>(this.OnSessionOfflineReq);

            return true;
        }
        public FFWorker BindHandler<T>(Pbmsg.ClientCmdDef cmd, PbHandler<T> method) where T : pb::IMessage, new()
        {
            CmdHandler ch = (Player player, string data) =>
            {
                T reqMsg = Util.String2Pb<T>(data);
                method(player, reqMsg);
            };
            m_dictCmd2Func[(int)cmd] = ch;
            return this;
        }
        public void SendPlayerMsg<T>(Player player, Pbmsg.ServerCmdDef cmd, T pbMsgData) where T : pb::IMessage, new()
        {
            GateRouteMsgToSessionReq msgToSession = new GateRouteMsgToSessionReq() { Cmd = (Int16)cmd, Body = Util.Pb2String(pbMsgData) };
            msgToSession.Session_id.Add(player.nSessionID);
            m_ffrpc.Call(m_strDefaultGate, msgToSession);
        }
        public void BroadcastPlayerMsg<T>(Pbmsg.ServerCmdDef cmd, T pbMsgData) where T : pb::IMessage, new()
        {
            GateBroadcastMsgToSessionReq msgToSession = new GateBroadcastMsgToSessionReq() { Cmd = (Int16)cmd, Body = Util.Pb2String(pbMsgData) };

            m_ffrpc.Call(m_strDefaultGate, msgToSession);
        }
        public void ClosePlayer(Player player)
        {
            GateCloseSessionReq msgToSession = new GateCloseSessionReq() { Session_id = player.nSessionID };
            m_ffrpc.Call(m_strDefaultGate, msgToSession);
        }
        public Role getRoleBySessionID(Int64 id)
        {
            if (m_dictRoles.ContainsKey(id) == false)
            {
                return null;
            }
            Role role = m_dictRoles[id];
            return role;
        }
        public Player getPlayerBySessionID(Int64 id)
        {
            Role role = getRoleBySessionID(id);
            if (role is Player)
            {
                return role as Player;
            }
            return null;
        }
        
        public EmptyMsgRet OnRouteLogicMsgReq(RouteLogicMsgReq reqMsg)
        {
            int cmd = reqMsg.Cmd;
            Int64 nSessionID = reqMsg.Session_id;
            FFLog.Trace(string.Format("worker RouteLogicMsgReq! {0} {1}", cmd, nSessionID));

            if (m_dictCmd2Func.ContainsKey(cmd) == false)
            {
                FFLog.Error(string.Format("worker cmd invalid! {0}", cmd));
                return RPC_NONE;
            }
            if (cmd == (int)Pbmsg.ClientCmdDef.CLogin)
            {
                Player playerOld = getPlayerBySessionID(nSessionID);
                if (playerOld != null)
                {
                    ClosePlayer(playerOld);
                    if (playerOld.nSessionID == nSessionID)
                    {
                        m_dictRoles.Remove(nSessionID);
                        FFLog.Error(string.Format("worker cmd invalid! {0} {1} login twice", cmd, nSessionID));
                        return RPC_NONE;
                    }
                }
                else
                {
                    Player playerNew = new Player() { nSessionID = nSessionID };
                    m_dictRoles[nSessionID] = playerNew;
                }
            }
            else
            {
                if (m_dictRoles.ContainsKey(nSessionID) == false)
                {
                    FFLog.Error(string.Format("worker cmd invalid! {0}, have not recv login msg", cmd));
                    return RPC_NONE;
                }
            }
            Player player = getPlayerBySessionID(nSessionID);
            m_dictCmd2Func[cmd](player, reqMsg.Body);

            return RPC_NONE;
        }
        public EmptyMsgRet OnSessionOfflineReq(SessionOfflineReq reqMsg)
        {
            Int64 nSessionID = reqMsg.Session_id;
            FFLog.Trace(string.Format("worker OnSessionOfflineReq! {0}", nSessionID));
            if (m_dictRoles.ContainsKey(nSessionID) == false)
            {
                return RPC_NONE;
            }

            Player player = getPlayerBySessionID(nSessionID);
            m_dictRoles.Remove(nSessionID);

            Pbmsg.LogoutRet retMsg = new Pbmsg.LogoutRet()
            {
                Id = player.getID(),
                Name = player.strName,
            };
            BroadcastPlayerMsg<Pbmsg.LogoutRet>(Pbmsg.ServerCmdDef.SLogout, retMsg);
            return RPC_NONE;
        }
        public void HandleLogin(Player player, Pbmsg.LoginReq reqMsg)
        {
            player.strName = reqMsg.Name;
            player.nLevel = 1;
            Pbmsg.LoginRet retMsg = new Pbmsg.LoginRet()
            {
                Id = player.getID(),
                Name = reqMsg.Name,
                Level = player.nLevel,
            };
            SendPlayerMsg<Pbmsg.LoginRet>(player, Pbmsg.ServerCmdDef.SLogin, retMsg);

            {
                Pbmsg.EnterMapRet enterMapRet = new Pbmsg.EnterMapRet()
                {
                    Id = player.getID(),
                    Name = player.strName,
                    Level = player.nLevel,
                    X = player.x,
                    Y = player.y,
                    Hp = player.hp,
                    Direction = player.direction,
                    ObjType = 0,
            };
                
                BroadcastPlayerMsg<Pbmsg.EnterMapRet>(Pbmsg.ServerCmdDef.SEnterMap, enterMapRet);
            }

            
            foreach (Role roleOther in m_dictRoles.Values)
            {
                if (roleOther.getID() == player.getID())
                {
                    continue;
                }
                Pbmsg.EnterMapRet enterMapRet = new Pbmsg.EnterMapRet()
                {
                    Id = roleOther.getID(),
                    Name = roleOther.strName,
                    Level = roleOther.nLevel,
                    X = roleOther.x,
                    Y = roleOther.y,
                    Hp = roleOther.hp,
                    ObjType = 0,
                    Direction = roleOther.direction
                };
                if (roleOther is Monster)
                {
                    enterMapRet.ObjType = 1;
                }

                if (roleOther.hp == 0)
                {
                    roleOther.hp = 100;
                    enterMapRet.Hp = roleOther.hp;
                    BroadcastPlayerMsg<Pbmsg.EnterMapRet>(Pbmsg.ServerCmdDef.SEnterMap, enterMapRet);
                }
                SendPlayerMsg<Pbmsg.EnterMapRet>(player, Pbmsg.ServerCmdDef.SEnterMap, enterMapRet);
                
            }
        }
        public void HandleRun(Player player, Pbmsg.RunReq reqMsg)
        {
            player.x = reqMsg.X;
            player.y = reqMsg.Y;
            player.direction = reqMsg.Direction;
            Pbmsg.RunRet runRet = new Pbmsg.RunRet()
            {
                Id = player.getID(),
                X = player.x,
                Y = player.y,
            };
            BroadcastPlayerMsg<Pbmsg.RunRet>(Pbmsg.ServerCmdDef.SRun, runRet);
        }
        public void HandleAttack(Player player, Pbmsg.AttackReq reqMsg)
        {
            Role roleTarget = getRoleBySessionID(reqMsg.Targetid);
            if (roleTarget == null)
            {
                return;
            }
            Pbmsg.AttackRet retMsg = new Pbmsg.AttackRet()
            {
                Id = player.getID(),
                Targetid = reqMsg.Targetid,
                Magicid = reqMsg.Magicid,
            };
            BroadcastPlayerMsg<Pbmsg.AttackRet>(Pbmsg.ServerCmdDef.SAttack, retMsg);
            Random rd = new Random();
            int hpChaneged = 1 + rd.Next() % 5;

            if (roleTarget.hp >= hpChaneged)
            {
                roleTarget.hp -= hpChaneged;
            }
            else
            {
                roleTarget.hp = 0;
            }
            int hpNow = roleTarget.hp;
            Pbmsg.HPChangedRet retMsg2 = new Pbmsg.HPChangedRet()
            {
                Id = roleTarget.getID(),
                Magicid = reqMsg.Magicid,
                ValCur = hpNow,
                ValChanged = hpChaneged,
            };
            BroadcastPlayerMsg<Pbmsg.HPChangedRet>(Pbmsg.ServerCmdDef.SHpChanged, retMsg2);

            if (roleTarget is Monster && hpChaneged <= 3)
            {
                Monster monster = roleTarget as Monster;
                monster.direction = (player.direction + 4) % 8;
                Pbmsg.AttackRet monsterAttackMsg = new Pbmsg.AttackRet()
                {
                    Id = monster.getID(),
                    Targetid = player.getID(),
                };
                BroadcastPlayerMsg<Pbmsg.AttackRet>(Pbmsg.ServerCmdDef.SAttack, monsterAttackMsg);

                roleTarget = player;
                if (roleTarget.hp >= hpChaneged)
                {
                    roleTarget.hp -= hpChaneged;
                }
                else
                {
                    roleTarget.hp = 0;
                }
                hpNow = roleTarget.hp;
                Pbmsg.HPChangedRet retMsg3 = new Pbmsg.HPChangedRet()
                {
                    Id = roleTarget.getID(),
                    ValCur = hpNow,
                    ValChanged = hpChaneged,
                };
                BroadcastPlayerMsg<Pbmsg.HPChangedRet>(Pbmsg.ServerCmdDef.SHpChanged, retMsg3);
            }
        }
    }
}
