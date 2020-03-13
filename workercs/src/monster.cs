using System;
using System.Net.Sockets;
using System.Collections.Generic;

using pb = global::Google.Protobuf;

namespace ff
{
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

    public class MonsterMgr
    {
        private static MonsterMgr gInstance = null;
        public static MonsterMgr Instance() {
            if (gInstance == null){
                gInstance = new MonsterMgr();
            }
            return gInstance;
        }
        protected int gCount = 0;
        protected long nLastAITick = 0;
        
        public bool Init()
        {
            //InitRobot();
            int nGenId = 10000;
            int num = 0;
            for (int i = 0; i < 3; ++i)
            {
                string strName = string.Format("尸霸{0}", i + 1);
                nGenId++; var mon = new Monster() { nSessionID = nGenId, strName = strName, x = 18 + 20 - (int)MapCfg.CenterX + i * 2, y = 28 + 30 - (int)MapCfg.CenterY - i, apprID = 69 };
                RoleMgr.Instance().AddRole(mon);
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("蓝魔{0}", i + 1);
                nGenId++; var mon = new Monster() { nSessionID = nGenId, strName = strName, x = 12 + 20 - (int)MapCfg.CenterX + i * 2, y = 32 + 30 - (int)MapCfg.CenterY - i, apprID = 102 };
                RoleMgr.Instance().AddRole(mon);
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("山魔{0}", i + 1);
                nGenId++; var mon = new Monster() { nSessionID = nGenId, strName = strName, x = 15 + 20 - (int)MapCfg.CenterX + i * 2, y = 40 + 30 - (int)MapCfg.CenterY - i, apprID = 103 };
                RoleMgr.Instance().AddRole(mon);
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("黑暗魔王{0}", i + 1);
                nGenId++; var mon = new Monster() { nSessionID = nGenId, strName = strName, x = 20 + 20 - (int)MapCfg.CenterX + i * 2, y = 40 + 30 - (int)MapCfg.CenterY - i, apprID = 104 };
                RoleMgr.Instance().AddRole(mon);
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("双足蜥蜴{0}", i + 1);
                nGenId++; var mon = new Monster() { nSessionID = nGenId, strName = strName, x = 25 + 20 - (int)MapCfg.CenterX + i * 2, y = 39 + 30 - (int)MapCfg.CenterY - i, apprID = 105 };
                RoleMgr.Instance().AddRole(mon);
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("变异蜘蛛{0}", i + 1);
                nGenId++; var mon = new Monster() { nSessionID = nGenId, strName = strName, x = 25 + 20 - (int)MapCfg.CenterX + i * 2, y = 28 + 30 - (int)MapCfg.CenterY - i, apprID = 106 };
                RoleMgr.Instance().AddRole(mon);
            }
            for (int i = 0; i < num; ++i)
            {
                string strName = string.Format("土妖{0}", i + 1);
                nGenId++; var mon = new Monster() { nSessionID = nGenId, strName = strName, x = 16 + 20 - (int)MapCfg.CenterX + i * 2, y = 30 - i, apprID = 100 };
                RoleMgr.Instance().AddRole(mon);
            }


            FFNet.TimeroutLoop(1000, this.HandleMonsterAI, "HandleMonsterAI");
            return true;
        }
        public void InitRobot()
        {
            int centerX = 24;
            int centerY = 30;
            int nMaxNum = 30;
            for (int i = 0; i < nMaxNum; ++i)
            {
                if (i % 3 == 0)
                {
                    centerX = 24;
                    centerY = 30;
                }
                else if (i % 3 == 0)
                {
                    centerX = 29;
                    centerY = 21;
                }
                else
                {
                    centerX = 30;
                    centerY = 21;
                }
                Player player = new Player() { nSessionID = i + 200000, strName = string.Format("机器人{0}", i+1), bIsRobot = true};
                Random rd = new Random();
                player.x = centerX + rd.Next(1, 10);
                player.y = centerY + rd.Next(1, 10);
                RoleMgr.Instance().AddRole(player);
            }
        }
        public void HandleMonsterAI()
        {
            long now = Util.GetNowTimeMs();
            if (now - this.nLastAITick < 800)
            {
                return;
            }
            this.nLastAITick = Util.GetNowTimeMs();
            RoleMgr.Instance().ForeachRole((Role role) =>{
                //if (gCount++ % 10 == 0)
                //{
                //    FFLog.Trace(string.Format("HandleMonsterAI! {0},hp:{1}", gCount, role.hp));
                //}
                if (role.hp <= 0)
                {
                    if (Util.GetNowTimeMs() - role.nLastAttackedTime < 5000 && gCount++ % 10 != 0)
                        return;
                    role.hp = role.maxhp;
                    Pbmsg.EnterMapRet enterMapRet = role.BuildEnterMsg();
                    FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SEnterMap, enterMapRet);
                    return;
                }
                if (!(role is Monster))
                {
                    if (role is Player)
                    {
                        Player playerTmp = role as Player;
                        if (playerTmp.bIsRobot)
                        {
                            HandleRobotAI(playerTmp);
                        }
                    }
                    return;
                }

                Monster monster = role as Monster;
                if (monster.nLastAttackedRoleID == 0)
                    return;
                Player player = RoleMgr.Instance().GetPlayerBySessionID(monster.nLastAttackedRoleID);
                if (player == null || player.hp == 0)
                {
                    monster.nLastAttackedRoleID = 0;
                    return;
                }
                int nDistance = Util.Distance(monster.x, monster.y, player.x, player.y);
                if (nDistance <= 1)
                {
                    if (Util.GetNowTimeMs() - monster.nLastAttackTime < 1000)
                        return;
                    monster.nLastAttackTime = Util.GetNowTimeMs();
                    Pbmsg.AttackRet monsterAttackMsg = new Pbmsg.AttackRet()
                    {
                        Id = monster.GetID(),
                        Targetid = player.GetID(),
                    };
                    FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SAttack, monsterAttackMsg);

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
                    FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SHpChanged, retMsg3);
                }
                else//！怪物寻路追打角色
                {
                    GamePoint nextPos = GameUtil.FindPath(new GamePoint(monster.x, monster.y), new GamePoint(player.x, player.y));
                    if (RoleMgr.Instance().GetRoleByPos(nextPos.x, nextPos.y) != null)
                    {
                        return;
                    }
                    monster.x = nextPos.x;
                    monster.y = nextPos.y;
                    Pbmsg.RunRet runRet = new Pbmsg.RunRet()
                    {
                        Id = monster.GetID(),
                        X = monster.x,
                        Y = monster.y,
                    };
                    FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SRun, runRet);
                }
            });
        }
        public Role GetTargetRobot(Player player)
        {
            if (player.nCurTarget == 0)
            {
                return null;
            }
            Role roleTarget = RoleMgr.Instance().GetRoleBySessionID(player.nCurTarget);
            if (roleTarget == null || roleTarget.hp == 0)// || !(roleTarget is Player))
            {
                player.nCurTarget = 0;
                return null;
            }
            Role ret = roleTarget;// as Player;
            return ret;
        }
        public void HandleRobotAI(Player player)
        {
            long nowtm = Util.GetNowTimeMs();
            if (nowtm - player.nLastRobotAITm < 1000)
            {
                return;
            }
            player.nLastRobotAITm = nowtm;
            Role targetRole = GetTargetRobot(player);
            if (targetRole == null)//!没有目标找一个离自己最近的目标
            {
                RoleMgr.Instance().ForeachRole((Role roleOther)=>{
                    if (roleOther == player || roleOther.hp == 0 || roleOther is Monster)
                    {
                        return;
                    }
                    if (targetRole != null && Util.Distance(player.x, player.y, targetRole.x, targetRole.y) <= Util.Distance(player.x, player.y, roleOther.x, roleOther.y))
                    {
                        return;
                    }
                    targetRole = roleOther;
                    player.nCurTarget = targetRole.GetID();
                });
            }
            int nDistance = Util.Distance(player.x, player.y, targetRole.x, targetRole.y);
            if (nDistance <= 1)
            {
                Pbmsg.AttackRet monsterAttackMsg = new Pbmsg.AttackRet()
                {
                    Magicid = 2,
                    Id = player.GetID(),
                    Targetid = targetRole.GetID(),
                };
                FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SAttack, monsterAttackMsg);

                int hpChaneged = 200;

                //if (targetRole.hp >= hpChaneged)
                //{
                //    targetRole.hp -= hpChaneged;
                //}
                //else
                //{
                //    targetRole.hp = 0;
                //    targetRole.nLastAttackedTime = Util.GetNowTimeMs();
                //}
                Pbmsg.HPChangedRet retMsg2 = new Pbmsg.HPChangedRet()
                {
                    Id = targetRole.GetID(),
                    Magicid = 2,
                    ValCur = targetRole.hp,
                    ValChanged = hpChaneged,
                };
                FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SHpChanged, retMsg2);
                return;
            }
            //!追击
            int nDir = GameUtil.CalDirection(player.x, player.y, targetRole.x, targetRole.y);
            GamePoint offsetPos = GameUtil.CalPointByDirLen(nDir, 1);
            player.x = player.x + offsetPos.x;
            player.y = player.y + offsetPos.y;
            Pbmsg.RunRet runRet = new Pbmsg.RunRet()
            {
                Id = player.GetID(),
                X = player.x,
                Y = player.y,
            };
            FFWorker.Instance().GateBroadcastMsg((int)Pbmsg.ServerCmdDef.SRun, runRet);
            return;
        }
    }
}