using System;
using System.Net.Sockets;
using System.Collections.Generic;

using pb = global::Google.Protobuf;

namespace ff
{
    public struct GamePoint
    {
        public GamePoint(int a = 0, int b = 0) { x = a; y = b; }
        public int x;
        public int y;
        public GamePoint clone()
        {
            return new GamePoint(x, y);
        }
    }
    enum Direction
    {
        UP = 0,
        UP_RIGHT = 1,
        RIGHT = 2,
        RIGHT_DOWN = 3,
        DOWN = 4,
        DOWN_LEFT = 5,
        LEFT = 6,
        LEFT_UP = 7
    }
    enum MapCfg
    {
        CenterX = 20,
        CenterY = 30
    }
    public class Role
    {
        public Int64 nSessionID;
        public string strName;
        public int nLevel;
        public int x;
        public int y;
        public int maxhp;
        public int hp;
        public int direction;
        public long nLastAttackedTime;
        public int apprID;
        public Role()
        {
            nSessionID = 0;
            strName = "";
            nLevel = 0;
            Random rd = new Random();
            x = (int)MapCfg.CenterX + rd.Next() % 5;
            y = (int)MapCfg.CenterY + rd.Next() % 5;
            maxhp = 1000;
            hp = maxhp;
            direction = 4;
            nLastAttackedTime = 0;
            apprID = 18 + rd.Next() % 2;
        }
        public Int64 GetID()
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
        public Int64 nLastAttackedRoleID;
        public long nLastAttackTm;
        public Monster() : base()
        {
            nLastAttackedRoleID = 0;
            nLastAttackTm = 0;
        }
    };
    public delegate void CmdHandler(Player p, byte[] data);
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
        protected int xOffset = 30;
        protected int yOffset = 40;
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
            int nGenId = 10000;
            int num = 0;
            for (int i = 0; i < 1; ++i)
            {
                string strName = string.Format("尸霸{0}", i + 1);
                nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = strName, x = 15 + 20 - (int)MapCfg.CenterX + i * 2, y = 35 + 30 - (int)MapCfg.CenterY - i, apprID = 69 };
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("蓝魔{0}", i + 1);
                nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = strName, x = 12 + 20 - (int)MapCfg.CenterX + i * 2, y = 32 + 30 - (int)MapCfg.CenterY - i, apprID = 102 };
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("山魔{0}", i + 1);
                nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = strName, x = 15 + 20 - (int)MapCfg.CenterX + i * 2, y = 40 + 30 - (int)MapCfg.CenterY - i, apprID = 103 };
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("黑暗魔王{0}", i + 1);
                nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = strName, x = 20 + 20 - (int)MapCfg.CenterX + i * 2, y = 40 + 30 - (int)MapCfg.CenterY - i, apprID = 104 };
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("双足蜥蜴{0}", i + 1);
                nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = strName, x = 25 + 20 - (int)MapCfg.CenterX + i * 2, y = 39 + 30 - (int)MapCfg.CenterY - i, apprID = 105 };
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("变异蜘蛛{0}", i + 1);
                nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = strName, x = 25 + 20 - (int)MapCfg.CenterX + i * 2, y = 28 + 30 - (int)MapCfg.CenterY - i, apprID = 106 };
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("土妖{0}", i + 1);
                nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = strName, x = 16 + 20 - (int)MapCfg.CenterX + i * 2, y = 30 - i, apprID = 100 };
            }
            //nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物2", x = 42 + xOffset, y = 27 + yOffset, apprID = 10002};
            //nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物3", x = 48 + xOffset, y = 19 + yOffset, apprID = 10003};
            //nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物4", x = 52 + xOffset, y = 31 + yOffset, apprID = 10004};
            //nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物5", x = 45 + xOffset, y = 45 + yOffset, apprID = 10005};
            //nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物6", x = 51 + xOffset, y = 50 + yOffset, apprID = 10006};
            //nGenId++; m_dictRoles[nGenId] = new Monster() { nSessionID = nGenId, strName = "大怪物7", x = 55 + xOffset, y = 55 + yOffset, apprID = 10007 };
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

            FFNet.Timerout(1000, this.HandleMonsterAI);
            return true;
        }
        public FFWorker BindHandler<T>(Pbmsg.ClientCmdDef cmd, PbHandler<T> method) where T : pb::IMessage, new()
        {
            m_dictCmd2Func[(int)cmd] = (Player player, byte[] data) =>
            {
                T reqMsg = Util.Byte2Pb<T>(data);
                method(player, reqMsg);
            };
            return this;
        }
        public void SendPlayerMsg<T>(Player player, Pbmsg.ServerCmdDef cmd, T pbMsgData) where T : pb::IMessage, new()
        {
            GateRouteMsgToSessionReq msgToSession = new GateRouteMsgToSessionReq() { Cmd = (Int16)cmd, Body = Util.Pb2Byte(pbMsgData) };
            msgToSession.SessionId.Add(player.nSessionID);
            m_ffrpc.Call(m_strDefaultGate, msgToSession);
        }
        public void BroadcastPlayerMsg<T>(Pbmsg.ServerCmdDef cmd, T pbMsgData) where T : pb::IMessage, new()
        {
            GateBroadcastMsgToSessionReq msgToSession = new GateBroadcastMsgToSessionReq() { Cmd = (Int16)cmd, Body = Util.Pb2Byte(pbMsgData) };

            m_ffrpc.Call(m_strDefaultGate, msgToSession);
        }
        public void ClosePlayer(Player player)
        {
            GateCloseSessionReq msgToSession = new GateCloseSessionReq() { SessionId = player.nSessionID };
            m_ffrpc.Call(m_strDefaultGate, msgToSession);
        }
        public Role GetRoleBySessionID(Int64 id)
        {
            if (m_dictRoles.ContainsKey(id) == false)
            {
                return null;
            }
            Role role = m_dictRoles[id];
            return role;
        }
        public Player GetPlayerBySessionID(Int64 id)
        {
            Role role = GetRoleBySessionID(id);
            if (role is Player)
            {
                return role as Player;
            }
            return null;
        }

        public EmptyMsgRet OnRouteLogicMsgReq(RouteLogicMsgReq reqMsg)
        {
            int cmd = reqMsg.Cmd;
            Int64 nSessionID = reqMsg.SessionId;
            //FFLog.Trace(string.Format("worker RouteLogicMsgReq! {0} {1}", cmd, nSessionID));

            if (m_dictCmd2Func.ContainsKey(cmd) == false)
            {
                FFLog.Error(string.Format("worker cmd invalid! {0}", cmd));
                return RPC_NONE;
            }
            if (cmd == (int)Pbmsg.ClientCmdDef.CLogin)
            {
                Player playerOld = GetPlayerBySessionID(nSessionID);
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
            Player player = GetPlayerBySessionID(nSessionID);
            m_dictCmd2Func[cmd](player, reqMsg.Body);

            return RPC_NONE;
        }
        public EmptyMsgRet OnSessionOfflineReq(SessionOfflineReq reqMsg)
        {
            Int64 nSessionID = reqMsg.SessionId;
            FFLog.Trace(string.Format("worker OnSessionOfflineReq! {0}", nSessionID));
            if (m_dictRoles.ContainsKey(nSessionID) == false)
            {
                return RPC_NONE;
            }

            Player player = GetPlayerBySessionID(nSessionID);
            m_dictRoles.Remove(nSessionID);

            Pbmsg.LogoutRet retMsg = new Pbmsg.LogoutRet()
            {
                Id = player.GetID(),
                Name = player.strName,
            };
            BroadcastPlayerMsg<Pbmsg.LogoutRet>(Pbmsg.ServerCmdDef.SLogout, retMsg);
            return RPC_NONE;
        }
        public Pbmsg.EnterMapRet BuildEnterMsg(Role role)
        {
            Pbmsg.EnterMapRet enterMapRet = new Pbmsg.EnterMapRet()
            {
                Id = role.GetID(),
                Name = role.strName,
                Level = role.nLevel,
                X = role.x,
                Y = role.y,
                Hp = role.hp,
                Direction = role.direction,
                ObjType = role is Player ? 0 : 1,
                ApprID = role.apprID,
            };
            return enterMapRet;
        }
        public void HandleLogin(Player player, Pbmsg.LoginReq reqMsg)
        {
            player.strName = reqMsg.Name;
            player.nLevel = 1;
            Pbmsg.LoginRet retMsg = new Pbmsg.LoginRet()
            {
                Id = player.GetID(),
                Name = reqMsg.Name,
                Level = player.nLevel,
            };
            SendPlayerMsg<Pbmsg.LoginRet>(player, Pbmsg.ServerCmdDef.SLogin, retMsg);
            for (int i = 0; i < 100; ++i)
            {
                SendPlayerMsg<Pbmsg.LoginRet>(player, Pbmsg.ServerCmdDef.SLogin, retMsg);
            }

            {
                Pbmsg.EnterMapRet enterMapRet = BuildEnterMsg(player);
                BroadcastPlayerMsg<Pbmsg.EnterMapRet>(Pbmsg.ServerCmdDef.SEnterMap, enterMapRet);
            }


            foreach (Role roleOther in m_dictRoles.Values)
            {
                if (roleOther.GetID() == player.GetID())
                {
                    continue;
                }
                if (roleOther.hp == 0)
                {
                    continue;
                }
                Pbmsg.EnterMapRet enterMapRet = BuildEnterMsg(roleOther);
                SendPlayerMsg<Pbmsg.EnterMapRet>(player, Pbmsg.ServerCmdDef.SEnterMap, enterMapRet);
                //System.Threading.Thread.Sleep(100);

            }
        }
        public void HandleRun(Player player, Pbmsg.RunReq reqMsg)
        {
            //System.Threading.Thread.Sleep(3000);
            player.x = reqMsg.X;
            player.y = reqMsg.Y;
            player.direction = reqMsg.Direction;
            Pbmsg.RunRet runRet = new Pbmsg.RunRet()
            {
                Id = player.GetID(),
                X = player.x,
                Y = player.y,
            };
            BroadcastPlayerMsg<Pbmsg.RunRet>(Pbmsg.ServerCmdDef.SRun, runRet);
        }
        public void HandleAttack(Player player, Pbmsg.AttackReq reqMsg)
        {
            Role roleTarget = GetRoleBySessionID(reqMsg.Targetid);
            if (roleTarget == null)
            {
                return;
            }
            Pbmsg.AttackRet retMsg = new Pbmsg.AttackRet()
            {
                Id = player.GetID(),
                Targetid = reqMsg.Targetid,
                Magicid = reqMsg.Magicid,
            };
            BroadcastPlayerMsg<Pbmsg.AttackRet>(Pbmsg.ServerCmdDef.SAttack, retMsg);
            Random rd = new Random();
            int hpChaneged = rd.Next(1, roleTarget.maxhp / 10);

            if (roleTarget.hp >= hpChaneged)
            {
                roleTarget.hp -= hpChaneged;
            }
            else
            {
                roleTarget.hp = 0;
            }
            Pbmsg.HPChangedRet retMsg2 = new Pbmsg.HPChangedRet()
            {
                Id = roleTarget.GetID(),
                Magicid = reqMsg.Magicid,
                ValCur = roleTarget.hp,
                ValChanged = hpChaneged,
            };
            BroadcastPlayerMsg<Pbmsg.HPChangedRet>(Pbmsg.ServerCmdDef.SHpChanged, retMsg2);
            roleTarget.nLastAttackedTime = Util.GetNowTimeMs();

            if (roleTarget is Monster)
            {
                Monster monster = roleTarget as Monster;
                monster.direction = (player.direction + 4) % 8;
                monster.nLastAttackedRoleID = player.GetID();
                if (monster.hp == 0)
                {
                    Pbmsg.LeaveMapRet leaveMsg = new Pbmsg.LeaveMapRet()
                    {
                        Id = roleTarget.GetID(),
                    };
                    BroadcastPlayerMsg<Pbmsg.LeaveMapRet>(Pbmsg.ServerCmdDef.SLeaveMap, leaveMsg);
                }
            }
        }
        public void HandleMonsterAI()
        {
            foreach (Role role in m_dictRoles.Values)
            {
                if (role.hp == 0)
                {
                    if (Util.GetNowTimeMs() - role.nLastAttackedTime < 5000)
                        continue;
                    role.hp = role.maxhp;
                    Pbmsg.EnterMapRet enterMapRet = BuildEnterMsg(role);
                    BroadcastPlayerMsg<Pbmsg.EnterMapRet>(Pbmsg.ServerCmdDef.SEnterMap, enterMapRet);
                    continue;
                }
                if (!(role is Monster))
                {
                    continue;
                }

                Monster monster = role as Monster;
                if (monster.nLastAttackedRoleID == 0)
                    continue;
                Player player = GetPlayerBySessionID(monster.nLastAttackedRoleID);
                if (player == null || player.hp == 0)
                {
                    monster.nLastAttackedRoleID = 0;
                    continue;
                }
                int nDistance = Util.Distance(monster.x, monster.y, player.x, player.y);
                if (nDistance <= 1)
                {
                    if (Util.GetNowTimeMs() - monster.nLastAttackedTime < 1000)
                        continue;
                    monster.nLastAttackedTime = Util.GetNowTimeMs();
                    Pbmsg.AttackRet monsterAttackMsg = new Pbmsg.AttackRet()
                    {
                        Id = monster.GetID(),
                        Targetid = player.GetID(),
                    };
                    BroadcastPlayerMsg<Pbmsg.AttackRet>(Pbmsg.ServerCmdDef.SAttack, monsterAttackMsg);

                    Random rd = new Random();

                    Role roleTarget = player;
                    int hpChaneged = rd.Next(1, roleTarget.maxhp / 10);
                    if (roleTarget.hp >= hpChaneged)
                    {
                        roleTarget.hp -= hpChaneged;
                    }
                    else
                    {
                        roleTarget.hp = 0;
                    }
                    roleTarget.nLastAttackedTime = Util.GetNowTimeMs();
                    Pbmsg.HPChangedRet retMsg3 = new Pbmsg.HPChangedRet()
                    {
                        Id = roleTarget.GetID(),
                        ValCur = roleTarget.hp,
                        ValChanged = hpChaneged,
                    };
                    BroadcastPlayerMsg<Pbmsg.HPChangedRet>(Pbmsg.ServerCmdDef.SHpChanged, retMsg3);
                }
                else//！怪物寻路追打角色
                {
                    GamePoint nextPos = FindPath(new GamePoint(monster.x, monster.y), new GamePoint(player.x, player.y));
                    monster.x = nextPos.x;
                    monster.y = nextPos.y;
                    Pbmsg.RunRet runRet = new Pbmsg.RunRet()
                    {
                        Id = monster.GetID(),
                        X = monster.x,
                        Y = monster.y,
                    };
                    BroadcastPlayerMsg<Pbmsg.RunRet>(Pbmsg.ServerCmdDef.SRun, runRet);
                }
            }
            FFNet.Timerout(800, this.HandleMonsterAI);
        }
        public int CalDirection(int srcx, int srcy, int destx, int desty)
        {
            double xoffet = destx - srcx;
            double yoffet = desty - srcy;
            //console.log('calDirection %d,%d,%d,%d, [%d,%d]', srcx, srcy, destx, desty, xoffet, yoffet);
            double tan30 = Math.Tan(Math.PI / 6);
            double tan60 = Math.Tan(Math.PI / 3);

            if (xoffet > 0)
            {
                double tanValue = Math.Abs(yoffet / xoffet);
                if (tanValue < tan30)
                {
                    return (int)Direction.RIGHT;
                }
                else if (tanValue < tan60)
                {
                    if (yoffet > 0)
                    {
                        return (int)Direction.RIGHT_DOWN;
                    }
                    return (int)Direction.UP_RIGHT;
                }
                else
                {
                    if (yoffet > 0)
                    {
                        return (int)Direction.DOWN;
                    }
                    return (int)Direction.UP;
                }
            }
            else if (xoffet < 0)
            {
                double tanValue = Math.Abs(yoffet / xoffet);
                if (tanValue < tan30)
                {
                    return (int)Direction.LEFT;
                }
                else if (tanValue < tan60)
                {
                    if (yoffet > 0)
                    {
                        return (int)Direction.DOWN_LEFT;
                    }
                    return (int)Direction.LEFT_UP;
                }
                else
                {
                    if (yoffet > 0)
                    {
                        return (int)Direction.DOWN;
                    }
                    return (int)Direction.UP;
                }
            }
            else
            {//xoffet == 0
                if (yoffet > 0)
                {
                    return (int)Direction.DOWN;
                }
                return (int)Direction.UP;
            }
        }
        public GamePoint CalPointByDirLen(int dir, int len)
        {
            int[,] cfgDirOffset = { { 0, -1 }, {1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };
            return new GamePoint(cfgDirOffset[dir, 0] * len, cfgDirOffset[dir, 1] * len);
        }
        public GamePoint FindPath(GamePoint fromPos, GamePoint destPos)
        {
            int nowX = fromPos.x;
            int nowY = fromPos.y;
            GamePoint tmpDestPos = destPos.clone();
            //!先直着走后斜着走的策略代码开始********************************************************************
            if (Math.Abs(destPos.x - nowX) != Math.Abs(destPos.y - nowY))
            {//!还没走到斜45度，那么就选一条常的边走
                int minOffset = Math.Min(Math.Abs(destPos.x - fromPos.x), Math.Abs(destPos.y - fromPos.y));
                if (destPos.x > fromPos.x)
                {
                    tmpDestPos.x -= minOffset;
                }
                else
                {
                    tmpDestPos.x += minOffset;
                }
                if (destPos.y > fromPos.y)
                {
                    tmpDestPos.y -= minOffset;
                }
                else
                {
                    tmpDestPos.y += minOffset;
                }
            }
            //!先直着走后斜着走的策略代码结束********************************************************************
            int dir = CalDirection(nowX, nowY, tmpDestPos.x, tmpDestPos.y);
            GamePoint destPoint = CalPointByDirLen(dir, 1);
            destPoint.x += nowX;
            destPoint.y += nowY;
            return destPoint;
        }
    }
}
