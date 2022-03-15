#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
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
 * @brief 请求队列向量
 * 
 * @param time 请求时间
 * @param demand_line 各客户端节点带宽需求向量
 */
typedef struct {
    tm time;
    vector<uint32_t> demand_line;
} demand_line_t;

/**
 * @brief 带宽需求表
 * 
 * @param header 客户节点ID向量
 * @param demand_list 请求队列向量 @see demand_line_t
 */
typedef struct {
    vector<string> header;
    vector<demand_line_t> demand_list;
} demand_table_t;

/**
 * @brief 边缘节点带宽上限
 * 
 * @param id_list 边缘节点ID向量
 * @param bandwidth_list 边缘节点带宽上限向量
 */
typedef struct {
    vector<string> id_list;
    vector<uint32_t> bandwidth_list;
} bandwidth_table_t;

typedef uint32_t qos_constraint_t;

/**
 * @brief 网络时延QoS表
 * 
 * @param row_id 行id
 * @param col_id 列id
 * @param qos_value Qos
 */
typedef struct {
    unordered_map<string, int, IdHash> row_id;
    unordered_map<string, int, IdHash> col_id;
    vector<vector<qos_constraint_t>> qos_value;
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
void DataLoader(demand_table_t *demand_table,
                bandwidth_table_t *bandwidth_table,
                qos_table_t *qos_table,
                qos_constraint_t *qos_constraint) {
    ifstream fs;
    string line_buffer;
    string element_buffer;
    stringstream ss;

    // read demand.csv
    fs.open("data/demand.csv", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer);
    ss = stringstream(line_buffer);
    getline(ss, element_buffer, ','); // ignore table name
    while (getline(ss, element_buffer, ',')) {
        demand_table->header.emplace_back(element_buffer);
    }
    while (getline(fs, line_buffer)) {
        demand_line_t demand_line_buffer;
        ss = stringstream(line_buffer);
        getline(ss, element_buffer, ',');
        sscanf(element_buffer.c_str(), "%d-%d-%dT%d:%d",
               &demand_line_buffer.time.tm_year,
               &demand_line_buffer.time.tm_mon,
               &demand_line_buffer.time.tm_mday,
               &demand_line_buffer.time.tm_hour,
               &demand_line_buffer.time.tm_min);
        while (getline(ss, element_buffer, ',')) {
            demand_line_buffer.demand_line.emplace_back(stoi(element_buffer));
        }
        demand_table->demand_list.emplace_back(demand_line_buffer);
    }
    fs.close();

    // read bandwidth
    fs.open("data/site_bandwidth.csv", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer); // ignore header
    while (getline(fs, line_buffer)) {
        ss = stringstream(line_buffer);
        getline(ss, element_buffer, ',');
        bandwidth_table->id_list.emplace_back(element_buffer);
        getline(ss, element_buffer);
        bandwidth_table->bandwidth_list.emplace_back(stoi(element_buffer));
    }
    fs.close();

    // read qos
    fs.open("data/qos.csv", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer);
    ss = stringstream(line_buffer);
    getline(ss, element_buffer, ','); // ignore table name
    for (int i = 0; getline(ss, element_buffer, ','); ++i) {
        qos_table->row_id[element_buffer] = i;
    }
    for (int i = 0; getline(fs, line_buffer); ++i) {
        vector<qos_constraint_t> qos_line_buffer;
        ss = stringstream(line_buffer);
        getline(ss, element_buffer, ',');
        qos_table->col_id[element_buffer] = i;
        while (getline(ss, element_buffer, ',')) {
            qos_line_buffer.emplace_back((qos_constraint_t)stoi(element_buffer));
        }
        qos_table->qos_value.emplace_back(qos_line_buffer);
    }
    fs.close();

    // read qos_constrain
    fs.open("data/config.ini", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer); // ignore header
    getline(fs, line_buffer);
    sscanf(line_buffer.c_str(), "qos_constraint=%d\n", qos_constraint);
    fs.close();
}

/**
 * @brief 分配方案写入到文件
 * 
 * @param allocate_table 分配表
 */
void DataOutput(allocate_table_t *allocate_table) {
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
 * @return double q百分位带宽值
 */
double getQuantile(vector<uint32_t> &x, double q=0.95){
    const int n = x.size();
    double id = ( n - 1 ) * q;
    int lo = floor(id);
    int hi = ceil(id);
    double qs = x[lo];
    double h = (id-lo);
    return (1.0 - h) * qs + h * x[hi];
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
void Calculate(demand_table_t *demand_table,
                bandwidth_table_t *bandwidth_table,
                qos_table_t *qos_table,
                qos_constraint_t *qos_constraint,
                allocate_table_t *allocate_table){

}

int main(int argc, char const *argv[]) {
    demand_table_t demand_table; // 客户节点带宽需求
    bandwidth_table_t bandwidth_table; // 边缘节点带宽上限
    qos_table_t qos_table; // 客户节点与边缘节点的网络时延
    qos_constraint_t qos_constraint; // QoS限制
    allocate_table_t allocate_table; // 分配表

    // 1. 数据预处理
    DataLoader(&demand_table, &bandwidth_table, &qos_table, &qos_constraint);
    // 2. 计算分配表
    // 3. 输出
    DataOutput(&allocate_table);

    return 0;
}