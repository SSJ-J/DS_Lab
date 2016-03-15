# DS_Lab
沈斯杰   5130379036  ds_ssj@sjtu.edu.cn

一 实现：
    此次的Lab1需要实现的是一个可信的MAC层，防止超时/乱序/出错，选择的方法是选择重发（与标准的略有不同，没有发nak）。
    首先对于上层待传的包，先放在window中，满了之后放在buffer中。对于时间timeout的设置，我是当每次窗口滑动时，对最早发的包设置一个timeout，在超时就重发这个最早发的包。
    处理乱序时，接收端也有一个window控制乱序，只要发送端不是发送需要的包时，接收端放在window中并返回前一个包的ack。比如，接收端要seq=5的包，收到了6,这时回发ack=4，表示5没收到，当5收到后，直接返回ack=6(或者更大，表示目前收到连续包的最大seq)。
    处理数据包出错时，发现checksum不对后就忽略这个包，当做超时处理。

二 性能：
    经测试发现，整个收发包的时间与TIMEOUT设置有关，TIMEOUT设置在0.1-0.3之间，TIMEOUT与性能正相关。
    开始使用16bits的校验码，发现错误率比较高，后来选用32bits，基本上不出错。
    其他提高性能的方法也与时钟的设置有关，具体见代码。

2016-3-15
