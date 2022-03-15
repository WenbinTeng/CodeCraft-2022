#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <string>
#include <unordered_map>

using namespace std;

typedef struct {
    tm time;
    vector<uint32_t> demand_line;
} demand_line_t;

typedef struct {
    vector<string> header;
    vector<demand_line_t> demand_list;
} demand_table_t;

typedef struct {
    vector<string> id_list;
    vector<uint32_t> bandwidth_list;
} bandwidth_table_t;

typedef struct {
    unordered_map<int, int> row_id;
    unordered_map<int, int> col_id;
    vector<vector<int>> qos_value;
} qos_table_t;

typedef uint32_t qos_constraint_t;

typedef struct {
    string custom_id;
    vector<string> site_id_list;
    vector<uint32_t> bandwidth_list;
} allocate_line_t;

typedef struct {
    vector<allocate_line_t> allocate_list;
} allocate_table_t;

int getIdHash(string& s) {
    return s.size() == 1 ? (int)s[0] : (int)s[1] << 8 | s[0];
}

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
        qos_table->row_id[getIdHash(element_buffer)] = i;
    }
    for (int i = 0; getline(fs, line_buffer); ++i) {
        vector<int> qos_line_buffer;
        ss = stringstream(line_buffer);
        getline(ss, element_buffer, ',');
        qos_table->col_id[getIdHash(element_buffer)] = i;
        while (getline(ss, element_buffer, ',')) {
            qos_line_buffer.emplace_back(stoi(element_buffer));
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

void DataSet(allocate_table_t *allocate_table) {
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

int main(int argc, char const *argv[])
{
    demand_table_t demand_table;
    bandwidth_table_t bandwidth_table;
    qos_table_t qos_table;
    qos_constraint_t qos_constraint;
    allocate_table_t allocate_table;

    DataLoader(&demand_table, &bandwidth_table, &qos_table, &qos_constraint);
    DataSet(&allocate_table);

    return 0;
}