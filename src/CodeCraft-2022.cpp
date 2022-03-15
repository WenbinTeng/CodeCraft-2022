#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>

using namespace std;

struct IdHash {
    size_t operator()(const string &s) const {
        return s.size() == 1 ? (size_t)s[0] : (size_t)s[1] << 8 | s[0];
    }
};

/**
 * @brief 带宽需求序列
 * 
 * @param id_map 客户节点ID下标映射
 * @param demand_list 请求队列向量 @see demand_line_t
 */
typedef struct {
    unordered_map<string, int, IdHash> id_map;
    vector<vector<uint32_t>> demand_value;
} demand_table_t;

/**
 * @brief 边缘节点带宽上限
 * 
 * @param id_map 边缘节点ID下标映射
 * @param bandwidth_value 边缘节点带宽上限向量
 */
typedef struct {
    unordered_map<string, uint32_t> bandwidth_value;
} bandwidth_table_t;

/**
 * @brief 网络时延QoS表
 * 
 * @param row_id_map 行ID下标映射
 * @param col_id_map 列ID下标映射
 * @param qos_value Qos
 */
typedef struct {
    unordered_map<string, int, IdHash> row_id_map;
    unordered_map<string, int, IdHash> col_id_map;
    vector<vector<uint32_t>> qos_value;
} qos_table_t;

/**
 * @brief 带宽调度分配方案
 * 
 * @param custom_id 客户端ID
 * @param site_id_list 
 * @param bandwidth_list 带宽需求向量
 */
typedef struct {
    string custom_id;
    vector<string> site_id_list;
    vector<uint32_t> bandwidth_list;
} allocate_line_t;

/**
 * @brief 带宽调度分配表
 * 
 * @param allocate_list 分配向量 
 */
typedef struct {
    vector<allocate_line_t> allocate_list;
} allocate_table_t;

/**
 * @brief 数据预处理
 * 
 * @param demand_table 客户节点带宽需求
 * @param bandwidth_table 边缘节点带宽上限
 * @param qos_table 客户节点与边缘节点的网络时延
 * @param qos_constraint QoS限制
 */
void dataLoader(demand_table_t *demand_table,
                bandwidth_table_t *bandwidth_table,
                qos_table_t *qos_table,
                uint32_t *qos_constraint) {
    ifstream fs;
    string line_buffer;
    string element_buffer;
    stringstream ss;

    // 读取 demand.csv
    fs.open("data/demand.csv", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer);
    ss = stringstream(line_buffer);
    getline(ss, element_buffer, ','); // 忽略表名
    for (int i = 0; getline(ss, element_buffer, ','); ++i) {
        demand_table->id_map[element_buffer] = i;
    }
    while (getline(fs, line_buffer)) {
        vector<uint32_t> demand_line_buffer;
        ss = stringstream(line_buffer);
        getline(ss, element_buffer, ','); // 忽略时间戳
        while (getline(ss, element_buffer, ',')) {
            demand_line_buffer.emplace_back(stoi(element_buffer));
        }
        demand_table->demand_value.emplace_back(demand_line_buffer);
    }
    fs.close();

    // 读取 site_bandwidth.csv
    fs.open("data/site_bandwidth.csv", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer); // 忽略表头
    for (int i = 0; getline(fs, line_buffer); ++i) {
        string id;
        ss = stringstream(line_buffer);
        getline(ss, id, ',');
        getline(ss, element_buffer);
        bandwidth_table->bandwidth_value[id] = stoi(element_buffer);
    }
    fs.close();

    // 读取 qos.csv
    fs.open("data/qos.csv", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer);
    ss = stringstream(line_buffer);
    getline(ss, element_buffer, ','); // 忽略表名
    for (int i = 0; getline(ss, element_buffer, ','); ++i) {
        qos_table->col_id_map[element_buffer] = i;
    }
    for (int i = 0; getline(fs, line_buffer); ++i) {
        vector<uint32_t> qos_line_buffer;
        ss = stringstream(line_buffer);
        getline(ss, element_buffer, ',');
        qos_table->row_id_map[element_buffer] = i;
        while (getline(ss, element_buffer, ',')) {
            qos_line_buffer.emplace_back(stoi(element_buffer));
        }
        qos_table->qos_value.emplace_back(qos_line_buffer);
    }
    fs.close();

    // 读取 qos_constrain
    fs.open("data/config.ini", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer); // 忽略头部
    getline(fs, line_buffer);
    sscanf(line_buffer.c_str(), "qos_constraint=%u\n", qos_constraint);
    fs.close();
}

/**
 * @brief 分配方案写入到文件
 * 
 * @param allocate_table 分配表
 */
void dataOutput(allocate_table_t *allocate_table) {
    ofstream fs;
    fs.open("output/solution.txt", ios::out);
    if (!fs)
        exit(-1);
    for (int i = 0; i < allocate_table->allocate_list.size(); ++i) {
        if (i > 0)
            fs << endl;
        fs << allocate_table->allocate_list[i].custom_id << ":";
        for (int j = 0; j < allocate_table->allocate_list[i].site_id_list.size(); ++j)
        {
            if (j > 0)
                fs << ",";
            fs << "<" << allocate_table->allocate_list[i].site_id_list[j] << "," << allocate_table->allocate_list[i].bandwidth_list[j] << ">";
        }
    }
    fs.close();
}

/**
 * @brief 获取百分位数
 * 
 * @param x 边缘节点带宽向量
 * @param q 百分位数
 * @return uint32_t q百分位带宽值
 */
uint32_t getQuantile(vector<uint32_t> &x, double q = 0.95) {
    const int n = x.size();
    int id = ceil(n * q);
    return x[id];
}

/**
 * @brief 计算分配表
 * 
 * @param demand_table 客户节点带宽需求
 * @param bandwidth_table 边缘节点带宽上限
 * @param qos_table 客户节点与边缘节点的网络时延
 * @param qos_constraint QoS限制
 * @param allocate_table 分配表
 */
void calculate(demand_table_t *demand_table,
                bandwidth_table_t *bandwidth_table,
                qos_table_t *qos_table,
                uint32_t *qos_constraint,
                allocate_table_t *allocate_table){

}

int main(int argc, char const *argv[]) {
    demand_table_t demand_table; // 客户节点带宽需求
    bandwidth_table_t bandwidth_table; // 边缘节点带宽上限
    qos_table_t qos_table; // 客户节点与边缘节点的网络时延
    uint32_t qos_constraint; // QoS限制
    allocate_table_t allocate_table; // 分配表

    // 1. 数据预处理
    dataLoader(&demand_table, &bandwidth_table, &qos_table, &qos_constraint);
    // 2. 计算分配表
    calculate(&demand_table, &bandwidth_table, &qos_table, &qos_constraint, &allocate_table);
    // 3. 输出
    dataOutput(&allocate_table);

    return 0;
}