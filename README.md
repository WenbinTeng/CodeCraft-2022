# CodeCraft-2022

2022华为软件精英挑战赛解法记录



## 解题方法

- 对请求带宽的节点队列进行轮询分配，分配片段为1500MB
- 对所有时刻的请求序列按照需求总和进行排序
- 对95百分位之前的带宽请求单独分配边缘节点
- 对95百分位之后的带宽请求均匀分配边缘节点



## 最终成绩

总成本400W，初赛排名40+，寄