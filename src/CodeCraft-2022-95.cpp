#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <string>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <random>
#include <chrono>

using namespace std;

// 文件路径 - Develop
string demand_file_path = "D:\\Users\\a1126\\Desktop\\CodeCraft-2022\\data\\demand.csv";
string site_bandwidth_file_path = "D:\\Users\\a1126\\Desktop\\CodeCraft-2022\\data\\site_bandwidth.csv";
string qos_file_path = "D:\\Users\\a1126\\Desktop\\CodeCraft-2022\\data\\qos.csv";
string config_file_path = "D:\\Users\\a1126\\Desktop\\CodeCraft-2022\\data\\config.ini";
string solution_file_path = "D:\\Users\\a1126\\Desktop\\CodeCraft-2022\\output\\solution.txt";
// 文件路径 - Product
// string demand_file_path = "/data/demand.csv";
// string site_bandwidth_file_path = "/data/site_bandwidth.csv";
// string qos_file_path = "/data/qos.csv";
// string config_file_path = "/data/config.ini";
// string solution_file_path = "/output/solution.txt";

int LEN;

struct IdHash {
    size_t operator()(const string &s) const {
        return s.size() == 1 ? (size_t)s[0] : (size_t)s[1] << 8 | s[0];
    }
};

/**
 * @brief 带宽需求序列
 * 
 * @param id_map 客户节点ID下标映射
 * @param demand_value 请求队列向量
 */
typedef struct {
    unordered_map<string, size_t, IdHash> id_map;
    vector<vector<uint32_t>> demand_value;
} demand_table_t;

/**
 * @brief 边缘节点带宽上限
 */
typedef unordered_map<string, uint32_t, IdHash> bandwidth_table_t;

/**
 * @brief 网络时延QoS表
 * 
 * @param server_id_map 行ID下标映射
 * @param client_id_map 列ID下标映射
 * @param qos_value Qos
 */
typedef struct {
    unordered_map<string, size_t, IdHash> server_id_map;
    unordered_map<string, size_t, IdHash> client_id_map;
    vector<vector<uint32_t>> qos_value;
} qos_table_t;

/**
 * @brief 带宽调度分配表
 */
typedef unordered_map<string, bandwidth_table_t, IdHash> allocate_table_t;

/**
 * @brief 数据预处理
 * 
 * @param demand_table 客户节点带宽需求
 * @param bandwidth_table 边缘节点带宽上限
 * @param qos_table 客户节点与边缘节点的网络时延
 * @param qos_constraint QoS限制
 */
void data_loader(demand_table_t &demand_table,
                bandwidth_table_t &bandwidth_table,
                qos_table_t &qos_table,
                uint32_t &qos_constraint) {
    ifstream fs;
    string line_buffer;
    string element_buffer;
    stringstream ss;

    // 读取 demand.csv
    fs.open(demand_file_path, ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer);
    ss = stringstream(line_buffer);
    getline(ss, element_buffer, ','); // 忽略表名
    for (int i = 0; getline(ss, element_buffer, ','); ++i) {
        demand_table.id_map[element_buffer] = i;
    }
    while (getline(fs, line_buffer)) {
        vector<uint32_t> demand_line_buffer;
        ss = stringstream(line_buffer);
        getline(ss, element_buffer, ','); // 忽略时间戳
        while (getline(ss, element_buffer, ',')) {
            demand_line_buffer.emplace_back(stoi(element_buffer));
        }
        demand_table.demand_value.emplace_back(demand_line_buffer);
    }
    fs.close();

    // 读取 site_bandwidth.csv
    fs.open(site_bandwidth_file_path, ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer); // 忽略表头
    for (int i = 0; getline(fs, line_buffer); ++i) {
        string id_buffer;
        ss = stringstream(line_buffer);
        getline(ss, id_buffer, ',');
        getline(ss, element_buffer);
        bandwidth_table[id_buffer] = stoi(element_buffer);
    }
    fs.close();

    // 读取 qos.csv
    fs.open(qos_file_path, ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer);
    ss = stringstream(line_buffer);
    getline(ss, element_buffer, ','); // 忽略表名
    for (int i = 0; getline(ss, element_buffer, ','); ++i) {
        qos_table.client_id_map[element_buffer] = i;
    }
    for (int i = 0; getline(fs, line_buffer); ++i) {
        vector<uint32_t> qos_line_buffer;
        ss = stringstream(line_buffer);
        getline(ss, element_buffer, ',');
        qos_table.server_id_map[element_buffer] = i;
        while (getline(ss, element_buffer, ',')) {
            qos_line_buffer.emplace_back(stoi(element_buffer));
        }
        qos_table.qos_value.emplace_back(qos_line_buffer);
    }
    fs.close();

    // 读取 qos_constrain
    fs.open(config_file_path, ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer); // 忽略头部
    getline(fs, line_buffer);
    sscanf(line_buffer.c_str(), "qos_constraint=%u\n", &qos_constraint);
    fs.close();
}

/**
 * @brief 分配方案写入到文件
 * 
 * @param allocate_table 分配表
 */
void data_output(vector<allocate_table_t> &allocate_tables) {
    ofstream fs;
    fs.open(solution_file_path, ios::out);
    if (!fs)
        exit(-1);
    for (int i = 0; i < allocate_tables.size(); ++i) {
        if (i > 0)
            fs << endl;
        for (const auto& [client_id, allocate_list] : allocate_tables[i]) {
            if (client_id != allocate_tables[i].begin()->first)
                fs << endl;
            for (auto c : client_id)
                if (c != '\n' && c != '\r')
                    fs << c;
            fs << ':';
            for (const auto& [server_id, allocate_bandwidth] : allocate_list) {
                if (server_id != allocate_list.begin()->first)
                    fs << ',';
                fs << '<';
                for (auto c : server_id)
                    if (c != '\n' && c != '\r')
                        fs << c;
                fs << ',' << allocate_bandwidth << '>';
            }
        }
    }
    fs.close();
}

/**
 * @brief 获取某一客户节点的可达边缘节点
 * 
 * @param client_id 客户节点ID
 * @param qos_table 
 * @param qos_constraint QoS限制
 * @return vector<string> 
 */
vector<string> get_valid_server(const string &client_id, qos_table_t &qos_table, uint32_t qos_constraint) {
    vector<string> ret;
    for (const auto [server_id, server_index] : qos_table.server_id_map) {
        if (qos_table.qos_value[server_index][qos_table.client_id_map[client_id]] < qos_constraint) {
            ret.emplace_back(server_id);
        }
    }
    return ret;
}

/**
 * @brief 从客户节点的可达边缘节点中获取最大剩余未访问节点
 * 
 * @param server_id 可达边缘节点列表
 * @param server_bandwidth 边缘节点总带宽
 * @return string 边缘节点ID
 */
string get_match_server(vector<string> &server_id, bandwidth_table_t &global_allocate_bandwidth, unordered_map<string, int, IdHash> &score_board, unordered_map<string, int, IdHash> &visit) {
    int idx = -1;
    int max_allocate = -1;
    // 已经使用的优先使用，使用次数在前5%之内
    for (int i = 0; i < server_id.size(); ++i) {
        if (max_allocate < (int)global_allocate_bandwidth[server_id[i]] && score_board[server_id[i]] < (int)(LEN * 0.05)) {
            max_allocate = global_allocate_bandwidth[server_id[i]];
            idx = i;
        }
    }
    if(idx != -1) return server_id[idx];
    // 使用次数在前5%的优先
    int min_len = __INT_MAX__;
    for (int i = 0; i < server_id.size(); ++i) {
        if (min_len > score_board[server_id[i]] && score_board[server_id[i]] < (int)(LEN * 0.05)) {
            min_len = score_board[server_id[i]];
            idx = i;
        }
    }
    if(idx != -1) return server_id[idx];
    // 如果所有可用节点都已经超过可用次数，则一直使用超过次数的那个节点？
    // int max_used = 0;
    // for (int i = 0; i < server_id.size(); ++i) {
    //     if (max_used < score_board[server_id[i]]) {
    //         max_used = score_board[server_id[i]];
    //         idx = i;
    //     }
    // }
    return server_id[server_id.size()-1];
}

/**
 * @brief 自动调整切片大小
 * 
 * @param demand 需求带宽
 * @param server_id 可达边缘节点列表
 * @param server_bandwidth 边缘节点总带宽
 * @return uint32_t 切片大小
 */
uint32_t get_auto_slice(uint32_t demand, vector<string> &server_id, bandwidth_table_t &server_bandwidth) {
    int cnt = 0;
    for (int i = 0; i < server_id.size(); ++i) {
        if (server_bandwidth[server_id[i]] > 0) {
            cnt++;
        }
    }
    return demand < cnt ? demand : demand / cnt ;
}

bool cmp(const pair<string, uint32_t> &x1, const pair<string, uint32_t> &x2){
    return x1.second > x2.second;
}

/**
 * @brief 计算某一时刻的分配方案
 * 
 * @param demand_table 总时刻请求表
 * @param demand_index 该时刻
 * @param qos_table Qos表
 * @param qos_constraint QoS限制
 * @param bandwidth_table 边缘节点带宽
 * @return allocate_table_t 
 */
allocate_table_t calculate_atime(demand_table_t &demand_table,
                                 uint32_t demand_index,
                                 qos_table_t &qos_table,
                                 uint32_t qos_constraint,
                                 bandwidth_table_t &bandwidth_table,
                                 unordered_map<string, int, IdHash>& score_board) {
    uint32_t slice = 100;
    allocate_table_t client_bandwidth; // 分配方案
    bandwidth_table_t server_bandwidth(bandwidth_table); // 拷贝边缘节点带宽
    queue<pair<string, uint32_t>> demand_queue;
    vector<pair<string, uint32_t>> demand_queue_unshuffle;
    unordered_map<string, vector<string>, IdHash> valid_server_list;
    bandwidth_table_t global_allocate_bandwidth;

    // 初始化当前时刻分配表
    for (const auto [id, _] : bandwidth_table) {
        global_allocate_bandwidth[id] = 0;
    }
    // 初始化分配方案、请求队列、可用边缘节点列表
    for (const auto& [client_id, client_index] : demand_table.id_map) {
        // 客户节点需求
        uint32_t client_demand = demand_table.demand_value[demand_index][client_index]; 
        client_bandwidth[client_id] = {};
        if (client_demand > 0) {
            demand_queue_unshuffle.push_back({client_id, client_demand});
            // demand_queue.push({client_id, client_demand});
            valid_server_list[client_id] = get_valid_server(client_id, qos_table, qos_constraint);
        }
    }
    // 根据请求大小由大到小排序
    sort(demand_queue_unshuffle.begin(), demand_queue_unshuffle.end(), cmp);
    // 构造请求队列
    for(int i=0; i<demand_queue_unshuffle.size(); i++){
        demand_queue.push({demand_queue_unshuffle[i].first, demand_queue_unshuffle[i].second});
    }
    // 设置访问数组,在无法保证5%的情况下,尽量不要重复访问
    unordered_map<string, int, IdHash> visit;
    // 遍历请求队列
    while (!demand_queue.empty()) {
        pair<string, uint32_t> front = demand_queue.front();
        demand_queue.pop();
        vector<string>& valid_server = valid_server_list[front.first];

        // 筛选可用节点中剩余空间不为0的节点
        for (int i = 0; i < valid_server.size(); ++i) {
            if (server_bandwidth[valid_server[i]] <= 0) {
                valid_server.erase(valid_server.begin() + i);
            }
        }
        
        string match_server = get_match_server(valid_server, global_allocate_bandwidth, score_board, visit);

        if(score_board[match_server] < (int)(LEN * 0.05)) slice = min(front.second, server_bandwidth[match_server]);
        else slice = get_auto_slice(front.second, valid_server, server_bandwidth);

        // 分配带宽
        slice = min(server_bandwidth[match_server], min(front.second, slice));
        client_bandwidth[front.first][match_server] += slice;
        global_allocate_bandwidth[match_server] += slice;
        front.second -= slice;
        server_bandwidth[match_server] -= slice;
        //访问
        visit[match_server]++;
        // 如果请求没有完全满足，再push回队列
        if(front.second > 0) demand_queue.push(front);
    }

    // 该时刻分配带宽的边缘节点计数+1
    for (const auto [id, bandwidth] : global_allocate_bandwidth) {
        if (bandwidth > 0) {
            score_board[id]++;
        }
    }

    return client_bandwidth;
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
vector<allocate_table_t> calculate(demand_table_t &demand_table,
                                   bandwidth_table_t &bandwidth_table,
                                   qos_table_t &qos_table,
                                   uint32_t qos_constraint,
                                   unordered_map<string, int, IdHash>& score_board) {
    vector<allocate_table_t> ret;
    // 初始化计分表
    for (const auto [id, _] : bandwidth_table) {
        score_board[id] = 0;
    }
    // 遍历每一组请求
    for (int i = 0; i < demand_table.demand_value.size(); ++i) {
        ret.emplace_back(calculate_atime(demand_table, i, qos_table, qos_constraint, bandwidth_table, score_board));
    }

    return ret;
}

int main(int argc, char const *argv[]) {
    demand_table_t demand_table; // 客户节点带宽需求
    bandwidth_table_t bandwidth_table; // 边缘节点带宽上限
    qos_table_t qos_table; // 客户节点与边缘节点的网络时延
    uint32_t qos_constraint; // QoS限制
    vector<allocate_table_t> allocate_tables; // 分配表
    unordered_map<string, int, IdHash> score_board; // 计分表

    // 1. 数据预处理
    data_loader(demand_table, bandwidth_table, qos_table, qos_constraint);
    LEN = demand_table.demand_value.size();
    // 2. 计算分配表
    allocate_tables = calculate(demand_table, bandwidth_table, qos_table, qos_constraint, score_board);
    // 3. 输出
    data_output(allocate_tables);

    return 0;
}