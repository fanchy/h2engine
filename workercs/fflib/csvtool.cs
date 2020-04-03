using System;
using System.Net.Sockets;
using System.IO;
using System.Text;
using System.Collections.Generic;

namespace ff
{
    public class RowDataCsv
    {
        public string[] lines;
        public RowDataCsv(string[] v)
        {
            lines = v;
        }
    }
    public class CsvTool
    {
        private List<RowDataCsv> m_listAllRow;
        public CsvTool()
        {
            m_listAllRow = new List<RowDataCsv>();
        }
        public bool LoadFromFile(string strFileName)
        {
            string[] lines = System.IO.File.ReadAllLines(strFileName);
            return LoadFromLines(lines);
        }
        public bool LoadFromStr(string data)
        {
            string[] lines = data.Split('\n');
            return LoadFromLines(lines);
        }
        public string[] SplitLine(string line)
        {
            List<string> ls = new List<string>();
            string str = "";
            for (int i = 0; i < line.Length; ++i)
            {
                if (line[i] == '"')//!找到下一个双引号
                {
                    ++ i;
                    while(i < line.Length)
                    {
                        var c = line[i];
                        if (c == '"')
                        {
                            break;
                        }
                        else
                        {
                            ++i;
                            str += c;
                        }
                    }
                }
                else if (line[i] == ',')
                {
                    ls.Add(str);
                    str = "";
                }
                else 
                {
                    str += line[i];
                }
            }
            if (str.Length > 0)
            {
                ls.Add(str);
                str = "";
            }

            return ls.ToArray();
        }
        public bool LoadFromLines(string[] lines)
        {
            m_listAllRow.Clear();
            if (lines.Length == 0)
            {
                return false;
            }
            string[] colNames = SplitLine(lines[0]);
            m_listAllRow.Add(new RowDataCsv(colNames));

            for (int i = 1; i < lines.Length; ++i)
            {
                string[] row = SplitLine(lines[i]);
                string[] rowDataLines = new string[colNames.Length];
                for (int j = 0; j < colNames.Length; ++j)
                {
                    if (j < row.Length)
                        rowDataLines[j] = row[j];
                    else
                        rowDataLines[j] = "";
                }
                m_listAllRow.Add(new RowDataCsv(rowDataLines));
            }
            return true;
        }
        public List<RowDataCsv> GetData()
        {
            return m_listAllRow;
        }
        public int rowLength
        {
            get { return m_listAllRow.Count; }
        }
        public int colLength
        {
            get
            {
                if (m_listAllRow.Count == 0)
                    return 0;
                return m_listAllRow[0].lines.Length;
            }
        }
        public string GetValue(int rowIndex, int colIndex)
        {
            if (rowIndex < 0 || colIndex < 0)
                return "";
            if (rowIndex < m_listAllRow.Count && colIndex < m_listAllRow[0].lines.Length)
            {
                return m_listAllRow[rowIndex].lines[colIndex];
            }
            return "";
        }
        public int GetColIndexByName(string strName)
        {
            if (m_listAllRow.Count == 0)
                return 0;
            for (int i = 0; i < m_listAllRow[0].lines.Length; ++i)
            {
                if (m_listAllRow[0].lines[i] == strName)
                    return i;
            }
            return -1;
        }
        public string[] GetRowData(int rowIndex)
        {
            if (rowIndex >= 0 && rowIndex < m_listAllRow.Count)
            {
                return m_listAllRow[rowIndex].lines;
            }
            return null;
        }
        public string[] GetColData(int colIndex)
        {
            if (m_listAllRow.Count == 0)
                return null;
            if (colIndex >= 0 && colIndex < m_listAllRow[0].lines.Length)
            {
                string[] ret = new string[m_listAllRow.Count];
                for (int i = 0; i < m_listAllRow.Count; ++i)
                {
                    ret[i] = m_listAllRow[i].lines[colIndex];
                }
                return ret;
            }
            return null;
        }
        public string[] GetColDataByName(string strName)
        {
            return GetColData(GetColIndexByName(strName));
        }
    }
}
/*
CsvTool csvTool = new CsvTool();
csvTool.LoadFromFile("D:/code/h2engine/workercs/test.csv");
string[] heads = csvTool.GetRowData(0);
string[] datas = csvTool.GetRowData(2);
string[] data2 = csvTool.GetColDataByName("key");
Console.WriteLine("headsp0={0}", heads[0]);
*/
