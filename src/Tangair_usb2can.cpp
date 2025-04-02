// # Copyright (c) 2023-2025 TANGAIR 
// # SPDX-License-Identifier: Apache-2.0
#include "Tangair_usb2can.h"


// 主函数循环
void Tangair_usb2can::Spin()
{
    while (all_thread_done_ != true)
    {
        sleep(1); // 延时1s
    }
    printf("~ ALL Exit ~\n");
}



/// @brief 构造函数，初始化
/// @return
Tangair_usb2can::Tangair_usb2can() 
{

    running_ = true;
    all_thread_done_ = false;

    USB2CAN0_ = openUSBCAN("/dev/USB2CAN0");
    if (USB2CAN0_ == -1)
        std::cout << std::endl
                  << "USB2CAN0 open INcorrect!!!" << std::endl;
    else
        std::cout << std::endl
                  << "USB2CAN0 opened ,num=" << USB2CAN0_ << std::endl;

    USB2CAN1_ = openUSBCAN("/dev/USB2CAN1");
    if (USB2CAN1_ == -1)
        std::cout << std::endl
                  << "USB2CAN1 open INcorrect!!!" << std::endl;
    else
        std::cout << std::endl
                  << "USB2CAN1 opened ,num=" << USB2CAN1_ << std::endl;

    // 电机ID配置
    USB2CAN_CAN_Bus_Init();

    
    // 启动成功
    std::cout << std::endl
              << "USB2CAN   NODE INIT__OK   by TANGAIR" << std::endl
              << std::endl
              << std::endl;

  
    // 创建CAN接收线程，设备1
    _CAN_RX_device_0_thread = std::thread(&Tangair_usb2can::CAN_RX_device_0_thread, this);

    // 创建CAN接收线程，设备2
    _CAN_RX_device_1_thread = std::thread(&Tangair_usb2can::CAN_RX_device_1_thread, this);

    //can 测试线程
    _CAN_TX_test_thread = std::thread(&Tangair_usb2can::CAN_TX_test_thread, this);

    //键盘输入线程
    _keyborad_input = std::thread(&Tangair_usb2can::keyborad_input, this);
}

/// @brief 析构函数
Tangair_usb2can::~Tangair_usb2can()
{

    running_ = false;

    /*注销线程*/

    // can接收设备0
    _CAN_RX_device_0_thread.join();
    // can接收设备1
    _CAN_RX_device_1_thread.join();

    //can发送测试线程
    _CAN_TX_test_thread.join();
    //键盘改速度
    _keyborad_input.join();

    // 失能电机
    DISABLE_ALL_MOTOR(100);

    // 关闭设备
    closeUSBCAN(USB2CAN0_);
    closeUSBCAN(USB2CAN1_);

    all_thread_done_ = true;
}



/// @brief can设备0，接收线程函数
void Tangair_usb2can::CAN_RX_device_0_thread()
{
    can_dev0_rx_count = 0;
    can_dev0_rx_count_thread=0;
    while (running_)
    {

        uint8_t channel;
        FrameInfo info_rx;
        uint8_t data_rx[8] = {0};

        can_dev0_rx_count_thread++;

        // 阻塞1s接收
        int recieve_re = readUSBCAN(USB2CAN0_, &channel, &info_rx, data_rx, 1e6);


        // 接收到数据
        if (recieve_re != -1)
        {
            can_dev0_rx_count++;
            // 解码
            CAN_DEV0_RX.ERR = data_rx[0]>>4&0X0F;
            
            CAN_DEV0_RX.current_position = (data_rx[1]<<8)|data_rx[2]; //电机位置数据
			CAN_DEV0_RX.current_speed  = (data_rx[3]<<4)|(data_rx[4]>>4); //电机速度数据
			CAN_DEV0_RX.current_torque = ((data_rx[4]&0xF)<<8)|data_rx[5]; //电机扭矩数据
			CAN_DEV0_RX.current_temp_MOS  = data_rx[6];
            CAN_DEV0_RX.current_temp_Rotor  = data_rx[7];
            // 转换
            CAN_DEV0_RX.current_position_f = uint_to_float(CAN_DEV0_RX.current_position, (P_MIN), (P_MAX), 16);
            CAN_DEV0_RX.current_speed_f = uint_to_float(CAN_DEV0_RX.current_speed, (V_MIN), (V_MAX), 12);    
            CAN_DEV0_RX.current_torque_f = uint_to_float(CAN_DEV0_RX.current_torque, (T_MIN), (T_MAX), 12);
  
     

            if (channel == 1) // 模块0，can1
            {
                switch (info_rx.canID)
                {
                case 0X11:
                {
                    USB2CAN0_CAN_Bus_1.ID_1_motor_recieve = CAN_DEV0_RX;
                   
                    break;
                }
                case 0X22:
                {
                    USB2CAN0_CAN_Bus_1.ID_2_motor_recieve = CAN_DEV0_RX;
                   
                    break;
                }
                case 0X33:
                {
                    USB2CAN0_CAN_Bus_1.ID_3_motor_recieve = CAN_DEV0_RX;
                    
                    break;
                }
             
                default:
                    break;
                }
            }
            else if (channel == 2) // 模块0，can2
            {
                switch (info_rx.canID)
                {
                case 0X11:
                {
                    USB2CAN0_CAN_Bus_2.ID_1_motor_recieve = CAN_DEV0_RX;
                  
                    break;
                }
                case 0X22:
                {
                    USB2CAN0_CAN_Bus_2.ID_2_motor_recieve = CAN_DEV0_RX;
                   
                    break;
                }
                case 0X33:
                {
                    USB2CAN0_CAN_Bus_2.ID_3_motor_recieve = CAN_DEV0_RX;
                   
                    break;
                }
               
                default:
                    break;
                }
            }
        }
    }
    std::cout << "CAN_RX_device_0_thread  Exit~~" << std::endl;
}

/// @brief can设备1，接收线程函数
void Tangair_usb2can::CAN_RX_device_1_thread()
{
    can_dev1_rx_count = 0;
    can_dev1_rx_count_thread=0;
    while (running_)
    {

        uint8_t channel;
        FrameInfo info_rx;
        uint8_t data_rx[8] = {0};

        // 接收有可能是CAN1，也有可能是CAN2，使用if进行分类
        int recieve_re = readUSBCAN(USB2CAN1_, &channel, &info_rx, data_rx, 1e6);

        can_dev1_rx_count_thread++;
        // 接收到数据
        if (recieve_re != -1)
        {
            can_dev1_rx_count++;
           // 解码
            CAN_DEV1_RX.ERR =data_rx[0]>>4&0X0F;
            
            CAN_DEV1_RX.current_position = (data_rx[1]<<8)|data_rx[2]; //电机位置数据
			CAN_DEV1_RX.current_speed  = (data_rx[3]<<4)|(data_rx[4]>>4); //电机速度数据
			CAN_DEV1_RX.current_torque = ((data_rx[4]&0xF)<<8)|data_rx[5]; //电机扭矩数据
			CAN_DEV1_RX.current_temp_MOS  = data_rx[6] ;
            CAN_DEV1_RX.current_temp_Rotor  = data_rx[7] ;
        
            // 转换
            CAN_DEV1_RX.current_position_f = uint_to_float(CAN_DEV1_RX.current_position, (P_MIN), (P_MAX), 16);
            CAN_DEV1_RX.current_speed_f = uint_to_float(CAN_DEV1_RX.current_speed, (V_MIN), (V_MAX), 12);    
            CAN_DEV1_RX.current_torque_f = uint_to_float(CAN_DEV1_RX.current_torque, (T_MIN), (T_MAX), 12);
                        

            if (channel == 1)  // 模块1，can1
            {
                switch (info_rx.canID)
                {
                case 0X11:
                {
                    USB2CAN1_CAN_Bus_1.ID_1_motor_recieve = CAN_DEV1_RX;
                    break;
                }
                case 0X22:
                {
                    USB2CAN1_CAN_Bus_1.ID_2_motor_recieve = CAN_DEV1_RX;
                    
                    break;
                }
                case 0X33:
                {
                    USB2CAN1_CAN_Bus_1.ID_3_motor_recieve = CAN_DEV1_RX;
                   
                    break;
                }
          
                default:
                    break;
                }
            }
            else if (channel == 2)  // 模块1，can2
            {
                switch (info_rx.canID)
                {
                case 0X11:
                {
                    USB2CAN1_CAN_Bus_2.ID_1_motor_recieve = CAN_DEV1_RX;
                   
                    break;
                }
                case 0X22:
                {
                    USB2CAN1_CAN_Bus_2.ID_2_motor_recieve = CAN_DEV1_RX;
                    
                    break;
                }
                case 0X33:
                {
                    USB2CAN1_CAN_Bus_2.ID_3_motor_recieve = CAN_DEV1_RX;
                    
                    break;
                }
          
                default:
                    break;
                }
            }
        }
    }
    std::cout << "CAN_RX_device_1_thread  Exit~~" << std::endl;
}


/*****************************************************************************************************/
/*********************************       ***测试相关***      ***********************************************/
/*****************************************************************************************************/
// can发送测试线程函数
void Tangair_usb2can::CAN_TX_test_thread()
{
  //发送计数
    uint32_t tx_count = 0;


   
    int can_dev0_rx_count_last=0;
    float max_band=0;
    int nice_time = 0;
    float nice_lost_rate = 0;
    std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> tpMill =
        std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    time_t tp_lst = tpMill.time_since_epoch().count();;
    //键盘输入速度
    speed_input = 190;
    //电机控制参数配置，单纯给速度
    {
        
        USB2CAN0_CAN_Bus_1.ID_1_motor_send.position = 0;
        USB2CAN0_CAN_Bus_1.ID_1_motor_send.speed = 2;
        USB2CAN0_CAN_Bus_1.ID_1_motor_send.torque = 0;
        USB2CAN0_CAN_Bus_1.ID_1_motor_send.kp = 0;
        USB2CAN0_CAN_Bus_1.ID_1_motor_send.kd = 1;

        USB2CAN0_CAN_Bus_1.ID_2_motor_send.position = 0;
        USB2CAN0_CAN_Bus_1.ID_2_motor_send.speed = 2;
        USB2CAN0_CAN_Bus_1.ID_2_motor_send.torque = 0;
        USB2CAN0_CAN_Bus_1.ID_2_motor_send.kp = 0;
        USB2CAN0_CAN_Bus_1.ID_2_motor_send.kd = 1;

        USB2CAN0_CAN_Bus_1.ID_3_motor_send.position = 0;
        USB2CAN0_CAN_Bus_1.ID_3_motor_send.speed = 2;
        USB2CAN0_CAN_Bus_1.ID_3_motor_send.torque = 0;
        USB2CAN0_CAN_Bus_1.ID_3_motor_send.kp = 0;
        USB2CAN0_CAN_Bus_1.ID_3_motor_send.kd = 1;

        USB2CAN0_CAN_Bus_2.ID_1_motor_send.position = 0;
        USB2CAN0_CAN_Bus_2.ID_1_motor_send.speed = 2;
        USB2CAN0_CAN_Bus_2.ID_1_motor_send.torque = 0;
        USB2CAN0_CAN_Bus_2.ID_1_motor_send.kp = 0;
        USB2CAN0_CAN_Bus_2.ID_1_motor_send.kd = 1;

        USB2CAN0_CAN_Bus_2.ID_2_motor_send.position = 0;
        USB2CAN0_CAN_Bus_2.ID_2_motor_send.speed = 2;
        USB2CAN0_CAN_Bus_2.ID_2_motor_send.torque = 0;
        USB2CAN0_CAN_Bus_2.ID_2_motor_send.kp = 0;
        USB2CAN0_CAN_Bus_2.ID_2_motor_send.kd = 1;

        USB2CAN0_CAN_Bus_2.ID_3_motor_send.position = 0;
        USB2CAN0_CAN_Bus_2.ID_3_motor_send.speed = 2;
        USB2CAN0_CAN_Bus_2.ID_3_motor_send.torque = 0;
        USB2CAN0_CAN_Bus_2.ID_3_motor_send.kp = 0;
        USB2CAN0_CAN_Bus_2.ID_3_motor_send.kd = 1;

        USB2CAN1_CAN_Bus_1.ID_1_motor_send.position = 0;
        USB2CAN1_CAN_Bus_1.ID_1_motor_send.speed = 2;
        USB2CAN1_CAN_Bus_1.ID_1_motor_send.torque = 0;
        USB2CAN1_CAN_Bus_1.ID_1_motor_send.kp = 0;
        USB2CAN1_CAN_Bus_1.ID_1_motor_send.kd = 1;

        USB2CAN1_CAN_Bus_1.ID_2_motor_send.position = 0;
        USB2CAN1_CAN_Bus_1.ID_2_motor_send.speed = 2;
        USB2CAN1_CAN_Bus_1.ID_2_motor_send.torque = 0;
        USB2CAN1_CAN_Bus_1.ID_2_motor_send.kp = 0;
        USB2CAN1_CAN_Bus_1.ID_2_motor_send.kd = 1;

        USB2CAN1_CAN_Bus_1.ID_3_motor_send.position = 0;
        USB2CAN1_CAN_Bus_1.ID_3_motor_send.speed = 2;
        USB2CAN1_CAN_Bus_1.ID_3_motor_send.torque = 0;
        USB2CAN1_CAN_Bus_1.ID_3_motor_send.kp = 0;
        USB2CAN1_CAN_Bus_1.ID_3_motor_send.kd = 1;

        USB2CAN1_CAN_Bus_2.ID_1_motor_send.position = 0;
        USB2CAN1_CAN_Bus_2.ID_1_motor_send.speed = 2;
        USB2CAN1_CAN_Bus_2.ID_1_motor_send.torque = 0;
        USB2CAN1_CAN_Bus_2.ID_1_motor_send.kp = 0;
        USB2CAN1_CAN_Bus_2.ID_1_motor_send.kd = 1;

        USB2CAN1_CAN_Bus_2.ID_2_motor_send.position = 0;
        USB2CAN1_CAN_Bus_2.ID_2_motor_send.speed = 2;
        USB2CAN1_CAN_Bus_2.ID_2_motor_send.torque = 0;
        USB2CAN1_CAN_Bus_2.ID_2_motor_send.kp = 0;
        USB2CAN1_CAN_Bus_2.ID_2_motor_send.kd = 1;

        USB2CAN1_CAN_Bus_2.ID_3_motor_send.position = 0;
        USB2CAN1_CAN_Bus_2.ID_3_motor_send.speed = 2;
        USB2CAN1_CAN_Bus_2.ID_3_motor_send.torque = 0;
        USB2CAN1_CAN_Bus_2.ID_3_motor_send.kp = 0;
        USB2CAN1_CAN_Bus_2.ID_3_motor_send.kd = 1;
    }
   
    //使能所有电机
    ENABLE_ALL_MOTOR(100);

    while (running_)
    {
         
        // CAN发送计数
        tx_count++;

         CAN_Send_Control(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_1_motor_send);
         std::this_thread::sleep_for(std::chrono::microseconds(speed_input));
          
        if (tx_count % 10000 == 0)
        {
            if(speed_input>100)
              speed_input--;  
        }
       
    
        std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tpMill =
            std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        time_t tp = tpMill.time_since_epoch().count();

        // 打印数据tp时间ms，1000hz的控制频率的话，1s一次，
        if (tx_count % 1000 == 0)
        {
           

            std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> tpMill =
            std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
            time_t tp = tpMill.time_since_epoch().count();

            std::cout << "can_tx_count=" << 1000 << "     " << "can_dev0_rx_count=" << can_dev0_rx_count-can_dev0_rx_count_last 
                      << "     "
                      << "loss rate=" <<(1-(float)(can_dev0_rx_count-can_dev0_rx_count_last )/1000.0)*100<< "%     "
                      <<std::endl
                      << "time_input=" <<speed_input<< "us" << "      "<<"time_real=" << (float)(tp - tp_lst)/1000 << "us" 
                      <<std::endl
                      << "Freq_target=" <<1000000/(float)speed_input<<""<< "HZ" << "      "<<"Freq_real=" << 1000000000.0/(float)(tp - tp_lst) << "HZ" ;
                      float band=((can_dev0_rx_count-can_dev0_rx_count_last )/1000.0)*1000000000.0/(float)(tp - tp_lst);
                      if(band>max_band){
                        max_band=band;
                        nice_time=(float)(tp - tp_lst)/1000; 
                        nice_lost_rate=(1-(float)(can_dev0_rx_count-can_dev0_rx_count_last )/1000.0)*100;
                      }

            std::cout <<"     band ="<<band<<"HZ"<< "    max_band="<<max_band<<"HZ"<<"     nice_time="<<nice_time<<"us"<<"     nice_lost_rate="<<nice_lost_rate<<"%"
                      << std::endl<< std::endl;

            tp_lst = tp;
            can_dev0_rx_count_last = can_dev0_rx_count;     
        }   
 

    }

    //程序终止时的提示信息
    std::cout << "CAN_TX_test_thread  Exit~~" << std::endl;
    std::cout << std::endl
              << "----------------请输入任意数字，按回车，以结束键盘进程  ----------------------" << std::endl
              << std::endl;
}

// 键盘输入线程，可以键盘修改电机速度
void Tangair_usb2can::keyborad_input()
{

    while (running_)
    {
        std::cin >> speed_input;
        std::cout << "speed_input=" << speed_input << std::endl;
    }
    std::cout << "keyborad_input_thread  Exit~~" << std::endl;
}

/*****************************************************************************************************/
/*********************************       ***电机相关***      ***********************************************/
/*****************************************************************************************************/

void Tangair_usb2can::USB2CAN_CAN_Bus_inti_set(USB2CAN_CAN_Bus_Struct *CAN_Bus)
{
    CAN_Bus->ID_1_motor_send.id = 0X01;
 

    
    CAN_Bus->ID_2_motor_send.id = 0X02;
 
    
    CAN_Bus->ID_3_motor_send.id = 0X03;



}

void Tangair_usb2can::USB2CAN_CAN_Bus_Init()
{
    USB2CAN_CAN_Bus_inti_set(&USB2CAN0_CAN_Bus_1);
    USB2CAN_CAN_Bus_inti_set(&USB2CAN0_CAN_Bus_2);
    USB2CAN_CAN_Bus_inti_set(&USB2CAN1_CAN_Bus_1);
    USB2CAN_CAN_Bus_inti_set(&USB2CAN1_CAN_Bus_2);
}

/// @brief 使能
/// @param dev
/// @param channel
/// @param Motor_Data
void Tangair_usb2can::Motor_Enable(int32_t dev, uint8_t channel, Motor_CAN_Send_Struct *Motor_Data)
{
    txMsg_CAN.canID = Motor_Data->id;

    Data_CAN[0] = 0xFF;
    Data_CAN[1] = 0xFF;
    Data_CAN[2] = 0xFF;
    Data_CAN[3] = 0xFF;
    Data_CAN[4] = 0xFF;
    Data_CAN[5] = 0xFF;
    Data_CAN[6] = 0xFF;
    Data_CAN[7] = 0xFC;

    sendUSBCAN(dev, channel, &txMsg_CAN, Data_CAN);
}

/// @brief 电机失能
/// @param dev
/// @param channel
/// @param Motor_Data
void Tangair_usb2can::Motor_Disable(int32_t dev, uint8_t channel, Motor_CAN_Send_Struct *Motor_Data)
{

    txMsg_CAN.canID = Motor_Data->id;

    Data_CAN[0] = 0xFF;
    Data_CAN[1] = 0xFF;
    Data_CAN[2] = 0xFF;
    Data_CAN[3] = 0xFF;
    Data_CAN[4] = 0xFF;
    Data_CAN[5] = 0xFF;
    Data_CAN[6] = 0xFF;
    Data_CAN[7] = 0xFD;


    sendUSBCAN(dev, channel, &txMsg_CAN, Data_CAN);
}

/// @brief 设置零点
/// @param dev
/// @param channel
/// @param Motor_Data
void Tangair_usb2can::Motor_Zore(int32_t dev, uint8_t channel, Motor_CAN_Send_Struct *Motor_Data)
{
    txMsg_CAN.canID = Motor_Data->id;

    Data_CAN[0] = 0xFF;
    Data_CAN[1] = 0xFF;
    Data_CAN[2] = 0xFF;
    Data_CAN[3] = 0xFF;
    Data_CAN[4] = 0xFF;
    Data_CAN[5] = 0xFF;
    Data_CAN[6] = 0xFF;
    Data_CAN[7] = 0xFE;


    sendUSBCAN(dev, channel, &txMsg_CAN, Data_CAN);
}

/// @brief 电机控制
/// @param dev 模块设备号
/// @param channel can1或者can2
/// @param Motor_Data 电机数据
void Tangair_usb2can::CAN_Send_Control(int32_t dev, uint8_t channel, Motor_CAN_Send_Struct *Motor_Data) // 运控�??,CAN1=CAN_TX_MAILBOX0,CAN2=CAN_TX_MAILBOX1
{
    // 运控模式专用的局部变
    FrameInfo txMsg_Control = {
        .canID = Motor_Data->id,
        .frameType = STANDARD,
        .dataLength = 8,
    };
    uint8_t Data_CAN_Control[8];

    //限制范围
    if(Motor_Data->kp>KP_MAX) Motor_Data->kp=KP_MAX;
        else if(Motor_Data->kp<KP_MIN) Motor_Data->kp=KP_MIN;
    if(Motor_Data->kd>KD_MAX ) Motor_Data->kd=KD_MAX;
        else if(Motor_Data->kd<KD_MIN) Motor_Data->kd=KD_MIN;
    if(Motor_Data->position>P_MAX)	Motor_Data->position=P_MAX;
        else if(Motor_Data->position<P_MIN) Motor_Data->position=P_MIN;
    if(Motor_Data->speed>V_MAX)	Motor_Data->speed=V_MAX;
        else if(Motor_Data->speed<V_MIN) Motor_Data->speed=V_MIN;
    if(Motor_Data->torque>T_MAX)	Motor_Data->torque=T_MAX;
        else if(Motor_Data->torque<T_MIN) Motor_Data->torque=T_MIN;


    Data_CAN_Control[0] = float_to_uint(Motor_Data->position, P_MIN, P_MAX, 16)>>8; //位置�?? 8
    Data_CAN_Control[1] = float_to_uint(Motor_Data->position, P_MIN, P_MAX, 16)&0xFF; //位置�?? 8
    Data_CAN_Control[2] = float_to_uint(Motor_Data->speed, V_MIN, V_MAX, 12)>>4; //速度�?? 8 �??
    Data_CAN_Control[3] = ((float_to_uint(Motor_Data->speed, V_MIN, V_MAX, 12)&0xF)<<4)|(float_to_uint(Motor_Data->kp, KP_MIN, KP_MAX, 12)>>8); //速度�?? 4 �?? KP �?? 4 �??
    Data_CAN_Control[4] = float_to_uint(Motor_Data->kp, KP_MIN, KP_MAX, 12)&0xFF; //KP �?? 8 �??
    Data_CAN_Control[5] = float_to_uint(Motor_Data->kd, KD_MIN, KD_MAX, 12)>>4; //Kd �?? 8 �??
    Data_CAN_Control[6] = ((float_to_uint(Motor_Data->kd, KD_MIN, KD_MAX, 12)&0xF)<<4)|(float_to_uint(Motor_Data->torque, T_MIN, T_MAX, 12)>>8); //KP �?? 4 位扭矩高 4 �??
    Data_CAN_Control[7] = float_to_uint(Motor_Data->torque, T_MIN, T_MAX, 12)&0xFF; //扭矩�?? 8

    int ret = sendUSBCAN(dev, channel, &txMsg_Control, Data_CAN_Control);

}

/// @brief 电机阻尼模式
/// @param dev
/// @param channel
/// @param Motor_Data
void Tangair_usb2can::Motor_Passive_SET(int32_t dev, uint8_t channel, Motor_CAN_Send_Struct *Motor_Data)
{
    Motor_Data->speed = 0;
    Motor_Data->kp = 0;
    Motor_Data->kd = 2.0;
    Motor_Data->torque = 0;

    CAN_Send_Control(dev, channel, Motor_Data);
}

void Tangair_usb2can::ENABLE_ALL_MOTOR(int delay_us)
{
    // 右前腿
    Motor_Enable(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Enable(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Enable(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Enable(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

    // 右前腿
    Motor_Enable(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Enable(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Enable(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Enable(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

    // 右前腿
    Motor_Enable(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Enable(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Enable(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Enable(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

   
}

void Tangair_usb2can::Tangair_usb2can::DISABLE_ALL_MOTOR(int delay_us)
{
    // 右前腿
    Motor_Disable(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Disable(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Disable(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Disable(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

    // 右前腿
    Motor_Disable(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Disable(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Disable(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Disable(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

    // 右前腿
    Motor_Disable(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Disable(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Disable(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Disable(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

 
}

void Tangair_usb2can::ZERO_ALL_MOTOR(int delay_us)
{

    // 右前腿
    Motor_Zore(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Zore(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Zore(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Zore(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

    // 右前腿
    Motor_Zore(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Zore(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Zore(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Zore(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

    // 右前腿
    Motor_Zore(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Zore(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Zore(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Zore(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

   
}

void Tangair_usb2can::PASSIVE_ALL_MOTOR(int delay_us)
{

    // 右前腿
    Motor_Passive_SET(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Passive_SET(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Passive_SET(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Passive_SET(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_1_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

    // 右前腿
    Motor_Passive_SET(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Passive_SET(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Passive_SET(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Passive_SET(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_2_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

    // 右前腿
    Motor_Passive_SET(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 右后腿
    Motor_Passive_SET(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左前腿
    Motor_Passive_SET(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us
    // 左后腿
    Motor_Passive_SET(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_3_motor_send);
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us)); // 单位us

 
}

/// @brief can控制发送，12个电机的数据
// 目前能达到1000hz的控制频率--------3000hz的总线发送频率---------同一路can的发送间隔在300us
void Tangair_usb2can::CAN_TX_ALL_MOTOR(int delay_us)
{
    auto t = std::chrono::high_resolution_clock::now();//这一句耗时50us

    //**ID_1_motor_send *//
    //右前腿
    CAN_Send_Control(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_1_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);
    //右后腿
    CAN_Send_Control(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_1_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);
    //左前腿
    CAN_Send_Control(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_1_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);
    //左后腿
    CAN_Send_Control(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_1_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);

    //**ID_2_motor_send *//
     // 右前腿
     CAN_Send_Control(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_2_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);
     // 右后腿
    CAN_Send_Control(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_2_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);
    //左前腿
    CAN_Send_Control(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_2_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);
    //左后腿
    CAN_Send_Control(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_2_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);

    //**ID_3_motor_send *//
    // 右前腿
    CAN_Send_Control(USB2CAN0_, 1, &USB2CAN0_CAN_Bus_1.ID_3_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);
    // 右后腿
    CAN_Send_Control(USB2CAN1_, 1, &USB2CAN1_CAN_Bus_1.ID_3_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);
    //左前腿
    CAN_Send_Control(USB2CAN0_, 2, &USB2CAN0_CAN_Bus_2.ID_3_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);
    //左后腿
    CAN_Send_Control(USB2CAN1_, 2, &USB2CAN1_CAN_Bus_2.ID_3_motor_send);
    t += std::chrono::microseconds(delay_us);
    std::this_thread::sleep_until(t);

    
}

/// @brief 辅助函数
int float_to_uint(float x, float x_min, float x_max, int bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    if (x > x_max)
        x = x_max;
    else if (x < x_min)
        x = x_min;
    return (int)((x - offset) * ((float)((1 << bits) - 1)) / span);
}
float uint_to_float(int x_int, float x_min, float x_max, int bits)
{
    /// converts unsigned int to float, given range and number of bits ///
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int) * span / ((float)((1 << bits) - 1)) + offset;
}

// # Copyright (c) 2023-2025 TANGAIR 
// # SPDX-License-Identifier: Apache-2.0

