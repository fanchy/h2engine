
using System;
using System.Net.Sockets;
using System.Collections.Generic;

using pb = global::Google.Protobuf;

namespace ff
{
    public class GamePoint
    {
        public GamePoint(int a = 0, int b = 0) { this.x = a; this.y = b; }
        public int x;
        public int y;
        public GamePoint Clone()
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

    public class GameUtil
    {
        public static int CalDirection(int srcx, int srcy, int destx, int desty)
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
        public static GamePoint CalPointByDirLen(int dir, int len)
        {
            int[,] cfgDirOffset = { { 0, -1 }, {1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };
            return new GamePoint(cfgDirOffset[dir, 0] * len, cfgDirOffset[dir, 1] * len);
        }
        public static GamePoint FindPath(GamePoint fromPos, GamePoint destPos)
        {
            int nowX = fromPos.x;
            int nowY = fromPos.y;
            GamePoint tmpDestPos = destPos.Clone();
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
