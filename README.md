# NFC-ST25
ST25DV04K和ST25R3911进行数据传输

设备分别是稚晖君原版L-ink card [用的微雪1.54 v2的屏幕和原版不一样]；微雪的NFC读写器；

一开始打算用快速传输模式，就是和手机nfc 传输数据用的那种；但是我还没搞出来，按照ST官方的demo。。。离谱；所以搞的读写st25dv的eeprom实现传输；[官方给的示例 不同示例注意底层文件是否一致 移植的时候注意下]

芯片用的ST25DV04K，按照手册通过RF天线能读写的块比较少，每次读写125块，也就是500字节，传输10次。。。很慢 25秒才传完；如果是大容量的芯片应该是可以只读写一次的，应该能到15秒钟。先这么多吧，后面在更新其他的

读写器资料：[ST25R3911B NFC Board - Waveshare Wiki](https://www.waveshare.net/wiki/ST25R3911B_NFC_Board)

ST25系列介绍：[ST25-NFC的个人空间_哔哩哔哩_bilibili](https://space.bilibili.com/517958317?from=search&seid=2453851323516222154)

ST25资料：

网盘资源（包含历年线下培训资料，本地化技术文档，技术FAQ等，链接:  链接: https://pan.baidu.com/s/1U4KjNytjTxIhOwAaW84BEw 提取码: 3j44） 
中文文档请参阅（文件描述是中文的文档为已经翻译的文档，更多文档翻译中）https://www.st.com/zh/nfc/st25-nfc-rfid-tags-readers.html
