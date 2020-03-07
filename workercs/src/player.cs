using System;
using System.Net.Sockets;
using System.Collections.Generic;

using pb = global::Google.Protobuf;

namespace ff
{
    public class Player: Role
    {
        public Player(): base()
        {
            playerYS = null;
            idZhuTi = 0;
            bIsRobot = false;
            nCurTarget = 0;
            nLastRobotAITm = 0;
            Random rd = new Random();
            if (rd.Next() % 2 == 0)
                apprID = 21;
            else
                apprID = 18;
        }
        public void SendPlayerMsg<T>(Pbmsg.ServerCmdDef cmd, T pbMsgData) where T : pb::IMessage, new()
        {
            Int64 nSessionID = this.nSessionID;
            Int16 nCmd = (Int16)cmd;
            if (this.idZhuTi != 0)
            {
                nSessionID = this.idZhuTi;
                //nCmd |= 0x4000;
            }
            FFWorker.Instance().SessionSendMsg(nSessionID, (Int16)cmd, pbMsgData);
        }
        public void ClosePlayer()
        {
            FFWorker.Instance().SessionClose(this.nSessionID);
        }
        public Player playerYS;//!元神
        public Int64 idZhuTi;//!元神对应的主体ID
        public bool bIsRobot;
        public Int64 nCurTarget;//!当前机器人选择的目标
        public long nLastRobotAITm;
    };
    public delegate void PbHandler<RET_TYPE>(Player p, RET_TYPE t);//泛型委托
    public class PlayerHandler
    {
        private static PlayerHandler gInstance = null;
        public static PlayerHandler Instance() {
            if (gInstance == null){
                gInstance = new PlayerHandler();
            }
            return gInstance;
        }
        public PlayerHandler()
        {
        }
        public bool Init()
        {
            FFWorker.Instance().BindHandler<Pbmsg.LoginReq>((int)Pbmsg.ClientCmdDef.CLogin, this.HandleLogin)
                .BindOffline(this.HandleLogout)
                .BindHandler<Player, Pbmsg.RunReq>((int)Pbmsg.ClientCmdDef.CRun, this.HandleRun)
                .BindHandler<Player, Pbmsg.AttackReq>((int)Pbmsg.ClientCmdDef.CAttack, this.HandleAttack)
                ;
            return true;
        }
        public void HandleLogin(Int64 nSessionID, Pbmsg.LoginReq reqMsg)
        {
            Player playerOld = RoleMgr.Instance().GetPlayerBySessionID(nSessionID);
            if (playerOld != null)
            {
                if (playerOld.nSessionID == nSessionID)
                {
                    FFLog.Error(string.Format("worker cmd invalid! {0} login twice",  nSessionID));
                }
                return;
            }
            
            Player player = new Player() { nSessionID = nSessionID };
            RoleMgr.Instance().AddRole(player);

            player.strName = reqMsg.Name;
            player.nLevel = 1;
            Pbmsg.LoginRet retMsg = new Pbmsg.LoginRet()
            {
                Id = player.GetID(),
                Name = reqMsg.Name,
                Level = player.nLevel,
            };
            player.SendPlayerMsg(Pbmsg.ServerCmdDef.SLogin, retMsg);

            {
                Pbmsg.EnterMapRet enterMapRet = player.BuildEnterMsg();
                FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SEnterMap, enterMapRet);

                player.playerYS = new Player() { nSessionID = player.nSessionID + 100000, strName = player.strName + "的元神", idZhuTi = player.GetID() };
                player.playerYS.x = player.x - 1;
                player.playerYS.y = player.y - 1;
                player.playerYS.apprID = player.apprID;
                RoleMgr.Instance().AddRole(player.playerYS);

                enterMapRet = player.playerYS.BuildEnterMsg();
                FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SEnterMap, enterMapRet);
            }

            RoleMgr.Instance().ForeachRole((Role roleOther) =>{
                if (roleOther.GetID() == player.GetID())
                {
                    return;
                }
                if (roleOther.hp == 0)
                {
                    return;
                }
                Pbmsg.EnterMapRet enterMapRet = roleOther.BuildEnterMsg();
                player.SendPlayerMsg(Pbmsg.ServerCmdDef.SEnterMap, enterMapRet);

            });
        }
        public void HandleLogout(Int64 nSessionID)
        {
            FFLog.Trace(string.Format("worker OnSessionOfflineReq! {0}", nSessionID));
            Player player = RoleMgr.Instance().GetPlayerBySessionID(nSessionID);
            if (player == null)
                return;
            RoleMgr.Instance().RemoveRole(nSessionID);

            Pbmsg.LogoutRet retMsg = new Pbmsg.LogoutRet()
            {
                Id = player.GetID(),
                Name = player.strName,
            };
            FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SLogout, retMsg);

            if (player.playerYS != null && RoleMgr.Instance().GetPlayerBySessionID(player.playerYS.nSessionID) != null)
            {
                RoleMgr.Instance().RemoveRole(player.playerYS.nSessionID);;
                retMsg = new Pbmsg.LogoutRet()
                {
                    Id = player.playerYS.GetID(),
                    Name = player.playerYS.strName,
                };
                FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SLogout, retMsg);
            }
            return;
        }
        public void HandleRun(Player player, Pbmsg.RunReq reqMsg)
        {
            player.x = reqMsg.X;
            player.y = reqMsg.Y;
            player.direction = reqMsg.Direction;
            Pbmsg.RunRet runRet = new Pbmsg.RunRet()
            {
                Id = player.GetID(),
                X = player.x,
                Y = player.y,
            };
            FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SRun, runRet);
        }
        public void HandleAttack(Player player, Pbmsg.AttackReq reqMsg)
        {
            Role roleTarget = RoleMgr.Instance().GetRoleBySessionID(reqMsg.Targetid);
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
            FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SAttack, retMsg);
            Random rd = new Random();
            int hpChaneged = rd.Next(1, roleTarget.maxhp / 10);
            //hpChaneged = 1;

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
            FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SHpChanged, retMsg2);
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
                    FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SLeaveMap, leaveMsg);
                }
            }
        }
    }
}
