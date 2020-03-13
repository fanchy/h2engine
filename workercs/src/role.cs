
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
        public int maxhp;
        public int hp;
        public int direction;
        public long nLastAttackedTime;
        public long nLastAttackTime;
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
            nLastAttackTime = 0;
            apprID = 18 + rd.Next() % 2;
        }
        public Int64 GetID()
        {
            return nSessionID;
        }
        public Pbmsg.EnterMapRet BuildEnterMsg()
        {
            Role role = this;
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
                OwnerID = 0,
            };
            if (role is Player)
            {
                Player player = role as Player;
                enterMapRet.OwnerID = player.idZhuTi;
            }
            return enterMapRet;
        }
    };
    public class RoleMgr{
        private static RoleMgr gInstance = null;
        public static RoleMgr Instance() {
            if (gInstance == null){
                gInstance = new RoleMgr();
            }
            return gInstance;
        }
        
        protected Dictionary<Int64, Role> m_dictRoles;
        RoleMgr()
        {
            m_dictRoles = new Dictionary<long, Role>();
        }
        public bool Init()
        {
            Player player = null;
            FFWorker.Instance().funcSessionID2Object = (Int64 s, int cmd, byte[] data)=>{
                Role ret = this.GetPlayerBySessionID(s);
                if (ret != null && ret is Player)
                {
                    player = ret as Player;
                    if ((cmd & 0x4000) != 0)
                    {
                        cmd &= ~(0x4000);
                        return player.playerYS;
                    }
                }
                return player;
            };
            return true;
        }
        public bool Cleanup()
        {
            return true;
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
        public void AddRole(Role role)
        {
            m_dictRoles[role.GetID()] = role;
        }
        public void RemoveRole(Int64 nSessionID)
        {
            m_dictRoles.Remove(nSessionID);
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
        public Player GetPlayerZhuTiByYuanShen(Player playerYS)
        {
            Role role = GetRoleBySessionID(playerYS.idZhuTi);
            if (role is Player)
            {
                return role as Player;
            }
            return null;
        }
        public Role GetRoleByPos(int x, int y, Int64 excludeID = 0)
        {
            foreach (Role roleOther in m_dictRoles.Values)
            {
                if (roleOther.GetID() == excludeID)
                {
                    continue;
                }
                if (roleOther.x == x && roleOther.y == y)
                {
                    return roleOther;
                }
            }
            return null;
        }
        public delegate void DelegateForeachRole(Role r);
        public void ForeachRole(DelegateForeachRole func)
        {
            foreach (Role roleOther in m_dictRoles.Values)
            {
                func(roleOther);
            }
        }
    }
}
