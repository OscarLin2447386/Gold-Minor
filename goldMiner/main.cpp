#include<iostream>
#include<vector>
#include<easyx.h>
#include<random>
#include <map>
#include<string>
#include<math.h>
#include<mmsystem.h>
#include<windows.h>
#include<sstream>
#include<Windows.h>
#include<fstream>
#include<thread>
#include <condition_variable>
#include <atomic>
#pragma comment(lib,"winmm.lib")
#include "EasyText.h"
#include "tools.h"
using namespace std;
#define USERDATA_FILE "userdata.txt" // string fileName （每行格式：用户名 密码 邮箱 昵称 分数）
#define WINDOW_HEIGHT 768// 窗口高度
#define WINDOW_WIDTH 1024//窗口宽度
#define PI 3.14159265359 //pai

//--------------------------------------数据设计------------------------------------------
/*
	bool gameStatus
	游戏状态: 0.失败 1.胜利
	记录当前游戏胜负状态
*/
bool gameStatus;
/*
	游戏内 游戏外
	音效状态；0.关闭 1.打开
	音乐状态；0.关闭 1.打开
*/
int gameSoundFlag = 1;
int systemSoundFlag = 1;
int systemMusicFlag = 1;
/*
	Item结构体:

		int number
		物品编号:1.黄金 2.钻石 3.石头

		pair<int, int> position
		物品坐标

		int size
		物品大小(三个功能):
		-决定了物品的边长(计算占用地图的面积范围)
		-决定了物品的大小(计算物品尺寸 例:大黄金 小黄金 大石头 小石头 小钻石等)
		-决定了物品的重量(计算钩子抓取后回缩速度)

		int score
		物品分数: 抓取该物品后玩家所得分数

		string image 物品图片路径
*/
struct Item {
	int number;
	int size;
	pair<int, int> position;
	int score;
};

/*
	Map结构体：

		int level（初始 Lv 1）;
		当前关卡难度
		-每次通关关卡难度++

		vector<Item> generated_Items
		地图物品列表:
		-当前地图随机生成的物品

		int height(568)
		地图高度:
		-限制物品生成范围

		int width(1024)
		地图宽度:
		-限制物品生成范围

		int goal
		目标分数: goal = level * 1500
		-在倒计时结束后判断游戏输赢

		int accumulated_score
		当前累计分数
		-在倒计时结束后判断游戏输赢

*/
struct Map {
	int level;
	vector<Item> generated_Items;
	int height;
	int width;
	int goal;
	int accumulated_score;
};

/*
	Hook结构体：

		int size = 55
		钩子大小：决定抓取范围

		int score;
		钩子抓回物品的分数

		pair<int, int> position
		钩子坐标：用于物品碰撞检测函数

		double speed;
		钩子（默认速度 = 7）出钩和回缩（更新后的速度）的速度
		60 FPS:  默认速度7 钩子出钩到达下边界只需1352ms(1.352秒)
		钩子速度vs实际时间表：
			6=1577ms	5=1893ms	4=2366ms	3=3155ms	2=4733ms

		int len;
		钩子摆动时距离原地的长度

		int status
		钩子状态；0.摆动 1.出钩 2.回缩

		int x, y;
		绳子的初始点

		int direction
		钩子摆动方向: （1）顺时针（-1）逆时针

		double angle
		钩子的摆动角度

		pair<double, double> v
		钩子摆动角度:
		-决定出钩后钩子的移动方向
		-第一个值代表每次移动在x的速率 x的速率根据角度调整
		-第二个值代表每次移动在y的速率 y的速率保持1不变

		Item getitem
		表示勾到的物品，若为NULL则表示空勾

*/
struct Hook {
	int size = 50;
	int score;
	pair<int, int> position;
	double speed;
	int len;
	int status;
	int x, y;
	int direction;
	double angle;//
	pair<int, int> v;
	Item getitem;
};

/*
	string username
	用户账号
	string password
	用户密码
	string name
	用户昵称
	int score
	用户获得的最高分数
	string email
	用户邮箱

	currentUser
	现在用户的信息
*/
struct Player {
	string username;//账号
	string password;//密码
	string email;//邮箱
	string name;//昵称
	int score;//最高分数
}currentUser, forget_player;

/*
	价值数组 元素下标对应物品编号 通过访问下标元素得到对应物品的价值
	用于计算生成物品的分数
	value[1] = 7（黄金）
	value[2] = 10（钻石）
	value[3] = 1（石头）
*/
const int value[4] = { 0,7,10,1 }; //全局变量

/*
	大小数组 元素下标对应小中大 通过访问下标元素得到对应物品的大小
	用于生成物品的大小
	sizes[1] = 20（小）
	sizes[2] = 45（中）
	sizes[3] = 90（大）
*/
const int sizes[4] = { 0,20,45,90 }; //全局变量
/*
	Time结构体:
		double miliseconds 当前倒计时毫秒数
		string image 倒计时木牌图片路径
*/
struct Time {
	int seconds;
}countdown;
/*
声明ExMessage全局变量，监听用户操作
*/
ExMessage msg = { 0 };

/*
	原版游戏主题
*/
const map <string, LPCTSTR> goldTheme{ //全局变量
	{"map", "./assests/goldTheme/background.png"},
	{"pause", "./assests/goldTheme/pause.png"},
	{"countdown", "./assests/goldTheme/未知"},
	{"person", "./assests/goldTheme/character.png"},
	{"hook", "./assests/goldTheme/hook.png"},
	{"gold", "./assests/goldTheme/gold.png"},
	{"diamond", "./assests/goldTheme/diamond.png"},
	{"stone", "./assests/goldTheme/stone.png"},
	{"goldhook", "./assests/goldTheme/goldhook.png"},
	{"diamondhook", "./assests/goldTheme/diamondhook.png"},
	{"stonehook", "./assests/goldTheme/stonehook.png"},
	{"sound", "./assests/goldTheme/sound.wav"},
	{"music", "./assests/goldTheme/music.mp3"},
	{"hookmask","./assests/goldTheme/hook_mask.png"},
	{"goldhookmask","./assests/goldTheme/goldhook_mask.png"},
	{"diamondhookmask","./assests/goldTheme/diamondhook_mask.png"},
	{"stonehookmask","./assests/goldTheme/stonehook_mask.png"}
};

/*
	嫦娥捞八戒游戏主题
*/
const map <string, LPCTSTR> changETheme{ //全局变量
	{"map", "./assests/changETheme/background.png"},
	{"pause", "./assests/changETheme/pause.png"},
	{"countdown", "./assests/changETheme/未知"},
	{"person", "./assests/changETheme/character.png"},
	{"hook", "./assests/changETheme/hook.png"},
	{"gold", "./assests/changETheme/gold.png"},
	{"diamond", "./assests/changETheme/diamond.png"},
	{"stone", "./assests/changETheme/stone.png"},
	{"goldhook", "./assests/changETheme/goldhook.png"},
	{"diamondhook", "./assests/changETheme/diamondhook.png"},
	{"stonehook", "./assests/changETheme/stonehook.png"},
	{"sound", "./assests/changETheme/sound."},
	{"music", "./assests/changETheme/music.mp3"},
	{"hookmask","./assests/changETheme/hook_mask.png"},
	{"goldhookmask","./assests/changETheme/goldhook_mask.png"},
	{"diamondhookmask","./assests/changETheme/diamondhook_mask.png"},
	{"stonehookmask","./assests/changETheme/stonehook_mask.png"}
};

/*
	末日黄昏游戏主题
*/
const map <string, LPCTSTR> apocalypseTheme{ //全局变量
	{"map", "./assests/apocalypseTheme/background.png"},
	{"pause", "./assests/apocalypseTheme/pause.png"},
	{"countdown", "./assests/apocalypseTheme/未知"},
	{"person", "./assests/apocalypseTheme/character.png"},
	{"hook", "./assests/apocalypseTheme/hook.png"},
	{"gold", "./assests/apocalypseTheme/gold.png"},
	{"diamond", "./assests/apocalypseTheme/diamond.png"},
	{"stone", "./assests/apocalypseTheme/stone.png"},
	{"goldhook", "./assests/apocalypseTheme/goldhook.png"},
	{"diamondhook", "./assests/apocalypseTheme/diamondhook.png"},
	{"stonehook", "./assests/apocalypseTheme/stonehook.png"},
	{"sound", "./assests/apocalypseTheme/sound.wav"},
	{"music", "./assests/apocalypseTheme/music.mp3"},
	{"hookmask","./assests/apocalypseTheme/hook_mask.png"},
	{"goldhookmask","./assests/apocalypseTheme/goldhook_mask.png"},
	{"diamondhookmask","./assests/apocalypseTheme/diamondhook_mask.png"},
	{"stonehookmask","./assests/apocalypseTheme/stonehook_mask.png"}
};

/*
	中秋节游戏主题
*/
const map <string, LPCTSTR> moonTheme{ //全局变量
	{"map", "./assests/moonTheme/background.png"},
	{"pause", "./assests/moonTheme/pause.png"},
	{"countdown", "./assests/moonTheme/未知"},
	{"person", "./assests/moonTheme/character.png"},
	{"hook", "./assests/moonTheme/hook.png"},
	{"gold", "./assests/moonTheme/gold.png"},
	{"diamond", "./assests/moonTheme/diamond.png"},
	{"stone", "./assests/moonTheme/stone.png"},
	{"goldhook", "./assests/moonTheme/goldhook.png"},
	{"diamondhook", "./assests/moonTheme/diamondhook.png"},
	{"stonehook", "./assests/moonTheme/stonehook.png"},
	{"sound", "./assests/moonTheme/sound.wav"},
	{"music", "./assests/moonTheme/music.mp3"},
	{"hookmask","./assests/moonTheme/hook_mask.png"},
	{"goldhookmask","./assests/moonTheme/goldhook_mask.png"},
	{"diamondhookmask","./assests/moonTheme/diamondhook_mask.png"},
	{"stonehookmask","./assests/moonTheme/stonehook_mask.png"}
};
/*
	万圣节游戏主题
*/
const map <string, LPCTSTR> halloweenTheme{ //全局变量
	{"map", "./assests/halloweenTheme/background.png"},
	{"pause", "./assests/halloweenTheme/pause.png"},
	{"countdown", "./assests/halloweenTheme/未知"},
	{"person", "./assests/halloweenTheme/character.png"},
	{"hook", "./assests/halloweenTheme/hook.png"},
	{"gold", "./assests/halloweenTheme/gold.png"},
	{"diamond", "./assests/halloweenTheme/diamond.png"},
	{"stone", "./assests/halloweenTheme/stone.png"},
	{"goldhook", "./assests/halloweenTheme/goldhook.png"},
	{"diamondhook", "./assests/halloweenTheme/diamondhook.png"},
	{"stonehook", "./assests/halloweenTheme/stonehook.png"},
	{"sound", "./assests/halloweenTheme/sound.wav"},
	{"music", "./assests/halloweenTheme/music.mp3"},
	{"hookmask","./assests/halloweenTheme/hook_mask.png"},
	{"goldhookmask","./assests/halloweenTheme/goldhook_mask.png"},
	{"diamondhookmask","./assests/halloweenTheme/diamondhook_mask.png"},
	{"stonehookmask","./assests/halloweenTheme/stonehook_mask.png"}
};
/*
	圣诞节游戏主题
*/
const map <string, LPCTSTR> christmasTheme{ //全局变量
	{"map", "./assests/christmasTheme/background.png"},
	{"pause", "./assests/christmasTheme/pause.png"},
	{"countdown", "./assests/christmasTheme/未知"},
	{"person", "./assests/christmasTheme/character.png"},
	{"hook", "./assests/christmasTheme/hook.png"},
	{"gold", "./assests/christmasTheme/gold.png"},
	{"diamond", "./assests/christmasTheme/diamond.png"},
	{"stone", "./assests/christmasTheme/stone.png"},
	{"goldhook", "./assests/christmasTheme/goldhook.png"},
	{"diamondhook", "./assests/christmasTheme/diamondhook.png"},
	{"stonehook", "./assests/christmasTheme/stonehook.png"},
	{"sound", "./assests/christmasTheme/sound.wav"},
	{"music", "./assests/christmasTheme/music.mp3"},
	{"hookmask","./assests/christmasTheme/hook_mask.png"},
	{"goldhookmask","./assests/christmasTheme/goldhook_mask.png"},
	{"diamondhookmask","./assests/christmasTheme/diamondhook_mask.png"},
	{"stonehookmask","./assests/christmasTheme/stonehook_mask.png"}
};

/*
	选择的游戏主题
	当游戏主题被更换，currentTheme会拷贝对应游戏主题的map
	通过 currentTheme["hook"] 就能访问到钩子当前主题的图片路径
*/
map <string, LPCTSTR> currentTheme = goldTheme;//全局变量
//--------------------------------------------------------------
/*
	按钮定义
*/
//登录界面
EasyTextBox txtName;
EasyTextBox txtPwd;
EasyButton btnLogIn;
EasyButton btnRegister;
EasyButton btnForgetPwd;
//忘记密码界面
EasyTextBox forgotPassView_txtName;
EasyTextBox forgotPassView_txtEmail;
EasyButton forgotPassView_btnConfirm;
EasyButton forgotPassView_btnReturn;
//重置密码界面
EasyTextBox txtNewpass;
EasyTextBox txtPass;
EasyButton resetPassView_btnConfirm;
EasyButton resetPassView_btnReturn;
//设置界面
EasyButton btnGameOn;
EasyButton btnGameOff;
EasyButton btnBkOn;
EasyButton btnBkOff;
EasyButton btnEsc;
//注册界面
EasyTextBox registerView_txtUsername;
EasyTextBox registerView_txtPlayername;
EasyTextBox registerView_txtEmail;
EasyTextBox registerView_txtPassword;
EasyTextBox registerView_txtConfirmPassword;
EasyButton registerView_btnConfirm;
EasyButton registerView_btnBack;
//多线程
mutex mtx;//互斥锁
condition_variable cv;//条件变量
atomic<bool> ready(false);//开始信号
atomic<bool> stopFlag(false);//停止信号
atomic<bool> reset(false);//重置时间信号

//--------------------------------------数据设计------------------------------------------

//--------------------------------------view------------------------------------------

/*
	登录界面
	负责人：阿熊
	功能：
		声明一个临时字符串string temp (用于指向指定的字符串)
		声明空字符串：用户名string username、密码string pass （用于储存用户输入）
		循环(true)：
			展示输入框：用户名 密码
			展示已输入的用户名和密码（密码用*代替）
			展示选项：1.登录 2.忘记密码 3.注册账号
			检测用户鼠标键盘输入：
				点击用户名框：temp = username
				键盘输入（读取单个字符）：
					用户名：每次检测到的字符输入加到用户名字符串temp末尾
					username = temp;
				点击密码框：temp = pass
				键盘输入（读取单个字符）：
					密码：每次检测到的字符输入加到密码字符串temp末尾
					pass = temp;
				登录：
					检索fileName文件（每行格式：用户名 密码 邮箱 昵称 分数）
					获取每行数据 确认是否存在匹配的用户名和密码
						成功：把用户名字符串拷贝到currentUser全局变量（用于当游戏结算时根据用户名记录分数）提示登陆成功 跳出循环
						失败：提示用户名和密码不匹配 请重新输入 清空用户名和密码字符串
				忘记密码：调用forgotPassView() 跳转忘记密码界面
				注册账号：调用registerView() 跳转注册账号界面
	参数：void
	返回类型：void
*/
void loginView();

/*
	忘记密码界面
	负责人：xxx
	功能：
		声明一个临时字符串string temp (用于指向指定的字符串)
		声明空字符串：用户名string username、邮箱string email （用于储存用户输入）
		循环(true)：
			展示输入框：用户名 邮箱
			展示已输入的用户名和邮箱
			展示选项 1.确定 2.返回登录界面
			检测玩家鼠标键盘输入：
				点击用户名框：temp = username
				键盘输入（读取单个字符）：
					用户名：每次检测到的字符输入加到用户名字符串temp末尾
					username = temp;
				点击邮箱框：temp = email
				键盘输入（读取单个字符）：
					邮箱：每次检测到的字符输入加到邮箱字符串temp末尾
					email = temp;
				确定：
					检索fileName文件（每行格式：用户名 密码 邮箱 昵称 分数 ）
					获取每行（用户名 邮箱）确认是否存在匹配的用户名和邮箱
						成功：调用resetPassView() 跳转重置密码界面 跳出循环 返回登录界面
						失败：提示用户名邮箱不匹配 请重新输入
				返回登录界面：跳出循环 返回登录界面
	参数：void
	返回类型：void
*/
void forgotPassView();

/*
	重置密码界面
	负责人：xxx
	功能：
		声明一个临时字符串string temp (用于指向指定的字符串)
		声明空字符串：密码string pass、确认密码string checkpass （用于储存用户输入）
		循环(true)：
			展示输入框：密码 确认密码
			展示已输入的密码和确认密码（字符*代替）
			展示选项 1.确定 2.返回登录界面
			检测玩家鼠标键盘输入：
				点击密码框：temp = pass
				键盘输入（读取单个字符）：
					密码：每次检测到的字符输入加到密码字符串temp末尾
					pass = temp;
				点击确认密码框：temp = checkpass
				键盘输入（读取单个字符）：
					确认密码：每次检测到的字符输入加到确认密码字符串temp末尾
					checkpass = temp;
				确认：
					验证密码格式是否符合要求（密码长度6-16位，包含字母和数字）
					验证确认密码是否一致
						成功：提示重置密码成功 更新fileName文件 跳出循环 返回登录界面
						失败：提示密码格式不正确或确认密码不一致 请重新输入
				返回登录界面：跳出循环 返回登录界面
	参数：void
	返回类型：void
*/

// 定义控件
void resetPassView();

/*
	注册账号界面
	负责人：xxx
	功能：
		声明一个临时字符串string temp (用于指向指定的字符串)
		声明空字符串：用户名string username 昵称string player 邮箱string email 密码string pass 确认密码string checkpass （用于储存用户输入)
		声明字符串:分数string score = 0 （默认最高分数=0）
		循环(true)：
			展示输入框：用户名 昵称 邮箱 密码 确认密码
			展示已输入用户名 昵称 邮箱 密码 确认密码（密码和确认密码用*代替）
			展示选项：1.确认 2.返回登录界面
			检测玩家鼠标键盘输入：
				点击用户名框：temp = username
				键盘输入（读取单个字符）：
					用户名：每次检测到的字符输入加到用户名字符串temp末尾
					username = temp;
				点击昵称框：temp = player
				键盘输入（读取单个字符）：
					昵称：每次检测到的字符输入加到昵称字符串temp末尾
					player = temp;
				点击邮箱框：temp = email
				键盘输入（读取单个字符）：
					邮箱：每次检测到的字符输入加到邮箱字符串temp末尾
					email = temp;
				点击密码框：temp = pass
				键盘输入（读取单个字符）：
					密码：每次检测到的字符输入加到密码字符串temp末尾
					pass = temp;
				点击确认密码框：temp = checkpass
				键盘输入（读取单个字符）：
					确认密码：每次检测到的字符输入加到确认密码字符串temp末尾
					checkpass = temp;
				确认：
					检索fileName文件（每行格式：用户名 密码 邮箱 昵称 分数）
					判断是否已存在：用户名 邮箱 昵称
					验证密码格式是否符合要求（密码长度6-16位，包含字母和数字）
					判断密码和确认密码是否一致
						成功：将用户名 密码 邮箱 昵称 分数放入fileName文件（每行格式：用户名 密码 邮箱 昵称 分数）
							  提示注册成功 跳出循环 返回登录界面
						失败：提示用户名邮箱密码格式不正确或确认密码不一致 请重新输入
				返回登录界面：跳出循环 返回登录界面
	参数：void
	返回类型：void
*/
void registerView();

/*
	主菜单界面
	负责人：曾
	功能：
		循环(true)：
			展示选项：1.开始游戏 2.游戏设置 3.排行榜 4.更换皮肤 5.游戏说明 6.退出游戏
			检测玩家输入：
				开始游戏：调用gameView() 跳转游戏界面
				游戏设置：调用settingView() 跳转游戏设置界面
				排行榜：调用rankView() 跳转排行榜界面
				更换皮肤：调用skinView() 跳转更换皮肤界面
				游戏说明：调用explainView() 跳转游戏说明界面
				退出游戏：调用exitGame() 退出程序
	参数：void
	返回类型：void
*/
void menuView();

/*
	游戏界面
	负责人：阿鑫
	功能：
		循环播放游戏背景音乐（音乐音频是currentTheme["music"]的值）
		实例化变量（游戏状态gameStatus、倒计时countdown、钩子hook、地图map）
		初始化地图的两个变量：关卡难度 = 1、累计分数 = 0
		调用initGame() 初始化游戏数据（游戏状态、倒计时、钩子、地图）
		循环(游戏状态 == true)：
			设置60FPS：
				double frameTime = 1000/60 （60帧所需的毫秒数）
				使用(int startTime = clock())开始计时
			调用renderGame() 渲染游戏界面
			调用swingHook() 钩子摆动
			使用(int endTime = clock()) 结束计时
			Time.milisecond -= frameTime
			sleep(frameTime - (endTime - startTime))
			检测玩家输入：
				下键：更新钩子状态（1.出钩） 调用launchHook() 发射钩子
				 ESC：调用pauseView() 暂停游戏
			判断(钩子状态== 2.回缩) 调用pullHook() 回缩钩子
			判断(倒计时 == 0)：
				判断(isWin() == true)：调用winGame()
				判断(isWin() == false)：调用loseGame()
	参数：void
	返回类型：void
*/
void gameView();

/*
	渲染游戏界面（调用eaxyX 的 putimage() 和 loadimage()）
	负责人：小滕 游戏界面（1024 * 768） 根据游戏布局确定每个图片的坐标
	功能： (图片储存在map<string,LPCTSTR> currentTheme)
	例：通过 currentTheme["hook"] 就能访问到当前主题的钩子图片路径
		渲染地图
		渲染暂停键
		渲染人物
		渲染倒计时
		渲染钩子 (坐标和大小在钩子结构体内)
		渲染物品 （坐标和大小在地图结构体内）
	参数：map<string, LPCTSTR>& currentTheme, Hook& hook, Map& map, Time& countdown
	返回类型：void
*/
void renderGame(map<string, LPCTSTR>& currentTheme, Hook& hook, Map& map, Time& countdown);
/*
	暂停界面
	负责人：阿魏
	功能：通过 currentTheme["sound"] 就能访问到当前主题的音效音频路径
		循环(true)：
			展示选项：1.继续游戏 2.返回菜单
			检测玩家鼠标输入：
				游戏音效(游戏内)；关/开 = 音量 0/100 :
					mciSendString("setaudio 音频路径 volume to 音量", NULL, 0, NULL);
					mciSendString("setaudio 音频路径 volume to 音量", NULL, 100, NULL);
				背景音乐(游戏内)：关/开 = 音量 0/100：
					mciSendString("setaudio 音频路径 volume to 音量", NULL, 0, NULL);
					mciSendString("setaudio 音频路径 volume to 音量", NULL, 100, NULL);
				继续游戏：跳出循环
				返回菜单：调用loseGame() 跳出循环
	参数：map<string, LPCTSTR>& currentTheme, bool& gameStatus, Map& map
	返回类型：void
*/
void pauseView(map<string, LPCTSTR>& currentTheme, bool& gameStatus, Map& map);

/*
	判断输赢
	负责人：阿魏
	功能：
		判断玩家积分是否达到要求
			胜利返回true
			失败返回false
	参数：Map &map
	返回类型：bool
*/
bool isWin(Map& map);
/*
	胜利界面
	负责人：阿魏
	功能：
		循环(true)：
			展示胜利弹窗 (显示当前分数)
			检测玩家鼠标输入 （点击下一关）
			跳出循环
	参数：Map &map
	返回类型：void
*/
void winView(Map& map);

/*
	失败界面
	负责人: 阿魏
	功能:
		循环(true)：
			展示失败界面 (显示当前分数)
			检测玩家鼠标输入
			点击返回菜单时
			跳出循环
	参数：Map &map
	返回类型：void
*/
void loseView(Map& map);

/*
	设置界面
	负责人：阿熊
	功能：
		循环(true)：
			展示选项：1.音效开关 2.音乐开关 3.返回菜单
			检测玩家鼠标输入：
				游戏音效(游戏外)；关/开 = 音量 0/100 :
					mciSendString("setaudio 音频路径(全局变量) volume to 音量", NULL, 0, NULL);
					mciSendString("setaudio 音频路径(全局变量) volume to 音量", NULL, 100, NULL);
				背景音乐(游戏外)：关/开 = 音量 0/100：
					mciSendString("setaudio 音频路径(全局变量) volume to 音量", NULL, 0, NULL);
					mciSendString("setaudio 音频路径(全局变量) volume to 音量", NULL, 100, NULL);
				返回菜单：跳出循环 返回主菜单
	参数：void
	返回类型：void
*/
void settingView();

/*
	排行榜界面 (string fileName （每行格式：用户名 密码 邮箱 昵称 分数）)
	负责人：xxx
	功能：
		声明一个map<int,string> rank
		遍历fileName文件，将分数和昵称装入rank (map容器自动根据键排序)
		循环(true)：
			展示排行榜：展示分数前十的玩家信息（昵称 分数）
			底部展示当前玩家信息（昵称 分数）
			检测玩家鼠标输入：
				返回菜单：跳出循环 返回主菜单
	参数：void
	返回类型：void
*/
void rankView();
/*
	更换皮肤界面
	负责人：xxx
	功能：
		循环(true)：
			展示游戏主题图片
			展示选项：1.多个游戏主题按钮 2.返回菜单
			检测玩家鼠标输入：
				主题按钮：根据用户的选择更换currentTheme的值
				返回菜单：跳出循环 返回主菜单
	参数：void
	返回类型：void
*/
void skinView();
/*
	游戏说明
	负责人：曾
	功能：
		循环(true)：
			展示游戏玩法
			展示游戏制作小组信息
			检测玩家鼠标输入：
				返回菜单：跳出循环 返回主菜单
	参数：void
	返回类型：void
*/
void explainView();
//--------------------------------------view------------------------------------------

//--------------------------------------service------------------------------------------
/*
	初始化游戏数据（游戏状态、倒计时、钩子、地图）（不初始化地图的关卡难度、累计分数）
	负责人：阿熊
	功能：
		游戏状态（true）
		初始化倒计时（6000毫秒）
		初始化钩子（原地位置、摆动状态、垂直向下角度、默认速度）
		地图：
			高度、宽度
			调用generateItems() 填充地图的随机物品数组
			目标分数 = 关卡难度 * 1500
	参数：bool &gameStatus, Time &countdown, Hook &hook, Map &map
	返回类型：void
*/
void initGame(bool& gameStatus, Time& countdown, Hook& hook, Map& map);

/*
	随机产生物品
	负责人：阿熊
	功能：
		清除随机物品数组的元素
		生成5个保底黄金 + 生成10个随机物品（1黄金 2钻石 3石头）
			物品编号：
				随机生成一个0~1之间的浮点数，0.01~0.40代表黄金 0.41~0.60表示钻石 0.61~1.0表示石头
				随着level增加，黄金占比减少 石头占比增加
				黄金占比减少：- (level * 0.01)
				石头占比增加：+ (level * 0.01)
				参考代码（加权随机选择）{
						vector<double> weights = {0.4 - level * 0.01, 0.2, 0.4 + level * 0.01}; 黄金 钻石 石头
						discrete_distribution<int> dist(weights.begin(), weights.end());
						random_device rd; 随机数引擎
						mt19937 gen(rd());
						int result = dist(gen)+1;  随机生成物品编号 1~3
				}
			物品大小：
				平均概率生成大小数组的下标 (1小 2中 3大)
				通过下标访问大小数组得到对应的大小 (size = sizes[下标])
			物品坐标：
				确保生成的坐标在地图范围内
				调用isEmpty() 判断生成的物品坐标是否已被占用
			物品分数：
				调用getScore() 计算物品分数
		将生成的15个物品放入地图中的随机物品数组
	参数：Map &map
	返回类型：void
*/
void generateItems(Map& map);

/*
	判断物品坐标是否已被占用
	负责人：阿熊
	功能：
		读取两个物品坐标和大小，判断是否会有部分重叠
		思路参考： https://blog.csdn.net/xikangsoon/article/details/80458591

	参数：pair<int,int> position1、pair<int,int> position2、int size1、int size2
	返回类型：bool
*/
bool isEmpty(pair<int, int> position1, pair<int, int> position2, int size1, int size2);

/*
	计算物品分数
	负责人：阿熊
	功能：
		分数 = value[物品编号] * size
	参数：Item &item
	返回类型：int
*/
int getScore(Item& item);

/*
	钩子摆动
	负责人：阿鑫
	功能：
		首先判断钩子摆动方向(1, -1)
		摆动角度 = 摆动度数*摆动方向
		钩子180度内来回摆动
		每次调用钩子摆动角度+1
		判断是否修改摆动方向
	参数：Hook &hook
	返回类型：void
*/
void swingHook(Hook& hook);

/*
	发射钩子
	负责人：阿鑫
	功能：
		循环(钩子状态 == 1.出钩)：
			设置60FPS：
				double frameTime = 1000/60 （60帧所需的毫秒数）
				使用(int startTime = clock())开始计时
			钩子根据当前角度前进并更新钩子坐标
			调用renderGame() 渲染新游戏画面
			调用collideDetect() 检测物品碰撞
			使用(int endTime = clock()) 结束计时
			Time.miliseconds -= frameTime
			sleep(frameTime - (endTime - startTime))
			判断(倒计时 == 0)：
				判断(isWin() == true)：调用winGame()
				判断(isWin() == false)：调用loseGame()
	参数：Time &countdown, Hook &hook, Map &map,bool gameStatus
	返回类型：void
*/
void launchHook(Time& countdown, Hook& hook, Map& map, bool& gameStatus);

/*
	碰撞检测
	负责人：阿鑫
	功能：
		检测钩子是否碰撞
			碰撞:  更新钩子状态（2.回缩）
				   Item数组中移除被抓取的物品
				   将获取的物品分数记录在Hook的score变量
				   调用updateHookSpeed() 更新钩子的速度（根据物品种类和大小）
				   更新钩子图片（钩子和物品合并）
	参数：Time &countdown, Hook &hook, Map &map
	返回类型：void
*/
void collideDetect(Time& countdown, Hook& hook, Map& map);

/*
	回缩钩子
	负责人：小滕
	功能：
		循环(钩子状态 == 2.回缩)：
			设置60FPS：
				double frameTime = 1000/60 （60帧所需的毫秒数）
				使用(int startTime = clock())开始计时
			钩子根据当前角度回缩并更新钩子坐标
			调用renderGame() 渲染新游戏画面
			判断是否到达原地
				到达原地：
					更新游戏分数(map)（根据物品的种类和大小）
					更新钩子速度（默认速度）
					更新钩子状态（0.摆动）
					更新钩子图片（原始图片）
			使用(int endTime = clock()) 结束计时
			Time.miliseconds -= frameTime
			sleep(frameTime - (endTime - startTime))
			判断(倒计时 == 0)：
				判断(isWin() == true)：调用winGame()
				判断(isWin() == false)：调用loseGame()
	参数：Time &countdown, Hook &hook, Map map,bool gameStatus
	返回类型：void
*/
void pullHook(Time& countdown, Hook& hook, Map& map, bool& gameStatus);

/*
	更新钩子速度（默认速度 = 7）
	负责人：小滕
	功能：
		根据物品的种类和大小更新钩子回缩速度：
			黄金（小-中-大）：速度（5-4-3）
			钻石（小-中-大）：速度（6-5-4）
			石头（小-中-大）：速度（5-4-3）
	参数：Hook &hook、Item &item
	返回类型：void
*/
void updateHookSpeed(Hook hook, Item item);
/*
	游戏胜利 (当 isWin() == true && countdown == 0 时调用)
	负责人：曾
	功能：
		更新游戏状态为true
		关卡难度++
		调用initGame() 初始化游戏数据（游戏状态、倒计时、钩子、地图） 开始下一关卡
		调用winView() 展示游戏胜利界面
	参数：bool gameStatus, Time countdown, Hook hook, Map map
	返回类型：void
*/
void winGame(bool& gameStatus, Time& countdown, Hook& hook, Map& map);

/*
	游戏失败 (当 isWin() == false && countdown == 0 时调用)
	负责人: 曾
	功能：
		更新游戏状态为false
		检索fileName文件 找到当前玩家的那行数据 （每行格式：用户名 密码 邮箱 昵称 分数）
		判断当前玩家的分数是否大于以往的最高分数：
			大于：更新当前玩家最高分数数值
		展示游戏失败界面(调用loseView)
	参数：bool& gameStatus, Map map
	返回类型：
*/
void loseGame(bool& gameStatus, Map& map);

/*
	负责人: 阿熊
	功能：
		用来反应btnLogIn按钮，判断数据库中是否有该用户
	参数：void
	返回类型：void

*/
void On_btnLogIn_Click();

/*

	负责人: 阿鑫
	功能：
		查找数据库，查找是否有username和email对应，若有返回true,否则返回false
	参数：string username, string email
	返回类型：bool
*/
bool findEmByUsername(string username, string email);
/*
	负责人: 阿鑫
	功能：
		查找数据库，通过用户名来查找该用户
	参数：string password
	返回类型：bool
*/
bool findPlayByUsername(string password);
/*

	负责人: 阿鑫
	功能：
		链接重置密码界面的确认按钮
	参数：void
	返回类型：void
*/
void On_resetPassView_btnConfirm_Click();

/*

	负责人: 阿鑫
	功能：
		用来反应btnConfirm按钮,确认之后进行判断是否有该用户和该邮箱
	参数：void
	返回类型：void

*/
// 按钮 forgotPassView_btnConfirm 的点击事件
void On_forgotPassView_btnConfirm_Click();
/*

	负责人: 阿鑫
	功能：
		用来反应btnReturn按钮,返回登录界面
	参数：void
	返回类型：void

*/
void On_forgotPassView_btnReturn_Click();

/*

	负责人: 阿鑫
	功能：
		用来反应resetPassView_btnReturn按钮,返回忘记密码界面
	参数：void
	返回类型：void

*/
void On_resetPassView_btnReturn_Click();

/*

	负责人: 阿熊
	功能：
		链接音效和音乐开关按钮
	参数：void
	返回类型：void

*/
void On_btnGameOn_Click();

void On_btnGameOff_Click();

void On_btnBkOn_Click();

void On_btnBkOff_Click();

/*

	负责人: 阿熊
	功能：
		链接设置界面的返回按钮，返回主菜单页面
	参数：void
	返回类型：void

*/
void On_btnEsc_Click();

/*
	注册界面链接函数
*/
void registerView_On_btnRegister_Click();

/*
	多线程实现倒计时功能
*/
void countDown(Time& countdown);

/*
	结束游戏
	负责人：曾
	功能：
		退出程序
	参数：void
	返回类型：void
*/
void exitGame();
//------------------------------------------------------------------界面-------------------------------------------------------------
void loginView()
{
	//播放主菜单音乐
	mciSendString("open gameLogin.mp3 alias musicLogin", NULL, 0, NULL);
	mciSendString("play musicLogin repeat", NULL, 0, NULL);
	mciSendString("setaudio musicLogin volume to 100", NULL, 0, NULL);

	cleardevice();
	BeginBatchDraw();
	IMAGE login;
	loadimage(&login, "./assests/login.png");
	putimage(0, 0, &login);

	//标题

	settextcolor(RGB(239, 218, 187));
	setbkmode(TRANSPARENT);//文本填充色：透明
	settextstyle(100, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型字魂无外润黑体(商用需授权)
	outtextxy(WINDOW_WIDTH / 2 - textwidth("黄金矿工") / 2, 200 - textheight("黄金矿工") / 2, "黄金矿工");//标题文本

	//登录界面显示
	settextcolor(RGB(239, 218, 187));//文本颜色：黑色
	settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
	setbkmode(TRANSPARENT);
	//solidrectangle(300, 310, 680, 350);
	outtextxy(278, 315, "用户名");
	//solidrectangle(300, 370, 680, 410);
	outtextxy(300, 375, "密码");

	settextcolor(BLACK);
	//setfillcolor(YELLOW);
	//solidrectangle(340, 480, 700, 520);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("登入") / 2, 480, "登入");
	btnLogIn.Create(470, 430, 575, 470, "登录", On_btnLogIn_Click);

	//solidrectangle(340, 540, 700, 580);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("登入") / 2, 540, "注册");
	btnRegister.Create(380, 500, 510, 540, "注册账号", registerView);

	//setfillcolor(WHITE);
	//settextstyle(30, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型字魂无外润黑体(商用需授权)
	//solidrectangle(620, 420, 740, 460);
	//outtextxy(640, 430, "忘记密码");
	btnForgetPwd.Create(530, 500, 660, 540, "忘记密码", forgotPassView);

	//输入框
	//setfillcolor(WHITE);
	//fillrectangle(370, 315, 750, 345); //账号输入框
	settextcolor(BLACK);
	settextstyle(20, 0, "字魂无外润黑体(商用需授权)");
	txtName.Create(370, 315, 700, 345, 20);
	//fillrectangle(370, 375, 750, 405); //密码输入框
	txtPwd.Create(370, 375, 700, 405, 20);

	//输入框前的贴图（？）
	//loadimage(Page[0], "账号图片路径");
	//putimage(0, 0, Page[0]);
	//loadimage(Page[0], "密码图片路径");
	//putimage(0, 0, Page[0]);
	EndBatchDraw();

	while (true)
	{
		msg = getmessage(EX_MOUSE);            // 获取消息输入

		if (msg.message == WM_LBUTTONDOWN)
		{
			// 判断控件
			if (btnLogIn.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				btnLogIn.OnMessage();
			}

			// 判断控件
			if (btnRegister.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				btnRegister.OnMessage();
			}

			// 判断控件
			if (btnForgetPwd.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				btnForgetPwd.OnMessage();
			}

			if (txtName.Check(msg.x, msg.y)) txtName.OnMessage();

			if (txtPwd.Check(msg.x, msg.y)) txtPwd.OnMessage();
		}
	}
}

int str_int(string s)
{
	int num = 0;
	for (int i = 0; i < s.length(); i++)
	{
		num = num * 10 + s[i] - '0';
	}
	return num;
}

void On_btnLogIn_Click()
{
	char* txtNameText = txtName.Text();
	char* txtPwdText = txtPwd.Text();
	string username(txtNameText);
	string password(txtPwdText);//强转string
	cout << "username:" << username << endl;
	cout << "password:" << password << endl;//测试
	//----------------------------------------打开文件-----------------------------------
	bool loginSuccess = false;

	FILE* fp;
	fopen_s(&fp, USERDATA_FILE, "r");
	if (fp == NULL) {
		cout << "无法打开文件!" << endl;
		return;
	}
	char line[256];
	while (fgets(line, sizeof(line), fp)) {
		string lineStr(line);
		// 去掉行末的换行符
		lineStr.erase(lineStr.find_last_not_of("\r\n") + 1);

		// 解析行数据
		size_t pos = 0;
		string token;
		string tokens[5]; // username, password, email, name, score
		int i = 0;

		while ((pos = lineStr.find(' ')) != string::npos) {
			token = lineStr.substr(0, pos);
			tokens[i++] = token;
			lineStr.erase(0, pos + 1);
		}
		tokens[i] = lineStr; // last part

		// 提取数据
		string fileAccount = tokens[0];
		string filePassword = tokens[1];

		if (fileAccount == username && filePassword == password) {
			currentUser.username = tokens[0];
			currentUser.password = tokens[1];
			currentUser.email = tokens[2];
			currentUser.name = tokens[3];
			currentUser.score = str_int(tokens[4]);//赋给currentUser
			loginSuccess = true;
			break;
		}
	}

	fclose(fp);

	if (loginSuccess) {
		cout << "登录成功!" << currentUser.name << endl;
		//调用menuView()
		mciSendString("close musicLogin", NULL, 0, NULL);
		menuView();
	}
	else {
		//cout << "用户名和密码不匹配，请重新输入。" << endl;
		MessageBox(GetHWnd(), "用户名和密码不匹配，请重新输入。", "错误", MB_OK);
		//清空界面
	}
}
void registerView_On_btnRegister_Click()
{
	// 获取输入框中的数据
	string username = registerView_txtUsername.Text();
	string playername = registerView_txtPlayername.Text();
	string email = registerView_txtEmail.Text();
	string password = registerView_txtPassword.Text();
	string confirmPassword = registerView_txtConfirmPassword.Text();

	// 检查用户名、昵称、邮箱、密码和确认密码
	if (username.empty() || playername.empty() || email.empty() || password.empty() || confirmPassword.empty())
	{
		MessageBox(GetHWnd(), "请输入所有字段", "错误", MB_OK);
		return;
	}

	if (password != confirmPassword)
	{
		MessageBox(GetHWnd(), "密码和确认密码不一致", "错误", MB_OK);
		return;
	}

	// 验证密码格式
	bool validPass = true;
	for (char c : password) {
		if (!isalnum(c)) {
			validPass = false;
			break;
		}
	}
	if (!validPass) {
		MessageBox(GetHWnd(), "密码格式不正确", "提示", MB_OK);
	}
	// 验证密码格式是否符合要求（密码长度6-16位，包含字母和数字）
	//if (password.length() < 6 || password.length() > 16 || !isalnum(password))
	//{
	//	ShowMessage( "密码格式不正确");
	//	return;
	//}

	// 检索fileName文件（每行格式：用户名 密码 邮箱 昵称 分数）
	ifstream file(USERDATA_FILE);
	string line;
	while (getline(file, line))
	{
		// 解析每行数据，并检查用户名和邮箱是否已经存在
		// 如果存在，提示用户用户名或邮箱已经存在
		string user, pass, mail, player, score;
		stringstream ss(line);
		ss >> user >> pass >> mail >> player >> score;
		if (user == username || mail == email || player == playername)
		{
			MessageBox(GetHWnd(), "用户名、邮箱或昵称已经存在", "错误", MB_OK);
			return;
		}
	}
	file.close();

	// 将用户名 密码 邮箱 昵称 分数放入fileName文件（每行格式：用户名 密码 邮箱 昵称 分数）
	ofstream outfile(USERDATA_FILE, ios::app);
	outfile << username << " " << password << " " << email << " " << playername << " 0" << endl;
	outfile.close();

	// 提示注册成功
	MessageBox(GetHWnd(), "注册成功", "提示", MB_OK);

	// 跳出循环 返回登录界面
	loginView();
}
void forgotPassView()
{
	BeginBatchDraw();
	cleardevice();
	//设置窗口颜色
	setbkcolor(RGB(255, 255, 255));
	cleardevice();

	IMAGE login;
	loadimage(&login, "./assests/forgotPass.png");
	putimage(0, 0, &login);

	settextcolor(RGB(239, 218, 187));
	setbkmode(TRANSPARENT);//文本填充色：透明
	settextstyle(100, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型字魂无外润黑体(商用需授权)
	outtextxy(WINDOW_WIDTH / 2 - textwidth("忘记密码") / 2, 200 - textheight("忘记密码") / 2, "忘记密码");//标题文本

	//登录界面显示
	settextcolor(RGB(239, 218, 187));//文本颜色：黑色
	settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
	setbkmode(TRANSPARENT);
	//solidrectangle(300, 310, 680, 350);
	outtextxy(278, 315, "用户名");
	//solidrectangle(300, 370, 680, 410);
	outtextxy(300, 375, "邮箱");

	settextcolor(BLACK);
	forgotPassView_btnReturn.Create(470, 490, 575, 530, "返回", On_forgotPassView_btnReturn_Click);
	forgotPassView_btnConfirm.Create(470, 430, 575, 470, "确认", On_forgotPassView_btnConfirm_Click);    // 创建确认控件

	//setfillcolor(WHITE);
	settextstyle(20, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型字魂无外润黑体(商用需授权)
	settextcolor(BLACK);
	//solidrectangle(370, 315, 770, 345);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("登入") / 2, 480, "登入");
	forgotPassView_txtName.Create(370, 315, 700, 345, 20);

	//solidrectangle(370, 375, 770, 405);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("登入") / 2, 540, "注册");
	forgotPassView_txtEmail.Create(370, 375, 700, 405, 20);
	EndBatchDraw();

	while (true)
	{
		msg = getmessage(EX_MOUSE);            // 获取消息输入

		if (msg.message == WM_LBUTTONDOWN)
		{
			// 判断控件
			if (forgotPassView_txtName.Check(msg.x, msg.y))forgotPassView_txtName.OnMessage();

			// 判断控件
			if (forgotPassView_txtEmail.Check(msg.x, msg.y))forgotPassView_txtEmail.OnMessage();

			// 判断控件
			if (forgotPassView_btnConfirm.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				forgotPassView_btnConfirm.OnMessage();
			}

			if (forgotPassView_btnReturn.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				forgotPassView_btnReturn.OnMessage();
			}
		}
	}

	// 关闭绘图窗口
	closegraph();

	getchar();
}

void registerView() {
	BeginBatchDraw();
	cleardevice();
	//设置窗口颜色
	setbkcolor(RGB(255, 255, 255));
	cleardevice();

	IMAGE login;
	loadimage(&login, "./assests/register.png");
	putimage(0, 0, &login);

	// 标题
	settextcolor(RGB(239, 218, 187));
	setbkmode(TRANSPARENT); // 文本填充色：透明
	settextstyle(100, 0, "字魂无外润黑体(商用需授权)"); // 字体大小&类型字魂无外润黑体(商用需授权)
	outtextxy(WINDOW_WIDTH / 2 - textwidth("注册账号") / 2, 200 - textheight("注册账号") / 2, "注册账号"); // 标题文本

	// 注册界面显示
	settextcolor(RGB(239, 218, 187)); // 文本颜色：黑色
	settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
	setbkmode(TRANSPARENT);
	//solidrectangle(300, 310, 680, 350);
	outtextxy(278, 315, "用户名");
	//solidrectangle(300, 370, 680, 410);
	outtextxy(300, 375, "昵称");
	//solidrectangle(300, 430, 680, 470);
	outtextxy(300, 435, "邮箱");
	//solidrectangle(300, 490, 680, 530);
	outtextxy(300, 495, "密码");
	//solidrectangle(300, 550, 680, 590);
	outtextxy(245, 555, "确认密码");

	settextcolor(BLACK);
	registerView_btnConfirm.Create(470, 610, 575, 650, "确认", registerView_On_btnRegister_Click);

	//setfillcolor(WHITE);

	//solidrectangle(620, 600, 740, 640);
	registerView_btnBack.Create(470, 670, 575, 710, "返回", loginView);

	settextstyle(20, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型字魂无外润黑体(商用需授权)
	settextcolor(BLACK);
	// 输入框
	registerView_txtUsername.Create(370, 315, 700, 345, 20);
	registerView_txtPlayername.Create(370, 375, 700, 405, 20);
	registerView_txtEmail.Create(370, 435, 700, 465, 20);
	registerView_txtPassword.Create(370, 495, 700, 525, 20);
	registerView_txtConfirmPassword.Create(370, 555, 700, 585, 20);

	EndBatchDraw();
	// 循环
	while (true)
	{
		ExMessage msg = getmessage(EX_MOUSE | EX_KEY);
		if (msg.message == WM_LBUTTONDOWN)
		{
			if (registerView_txtUsername.Check(msg.x, msg.y))
			{
				registerView_txtUsername.OnMessage();
			}
			else if (registerView_txtPlayername.Check(msg.x, msg.y))
			{
				registerView_txtPlayername.OnMessage();
			}
			else if (registerView_txtEmail.Check(msg.x, msg.y))
			{
				registerView_txtEmail.OnMessage();
			}
			else if (registerView_txtPassword.Check(msg.x, msg.y))
			{
				registerView_txtPassword.OnMessage();
			}
			else if (registerView_txtConfirmPassword.Check(msg.x, msg.y))
			{
				registerView_txtConfirmPassword.OnMessage();
			}
			else if (registerView_btnConfirm.Check(msg.x, msg.y))
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				registerView_btnConfirm.OnMessage();
			}
			else if (registerView_btnBack.Check(msg.x, msg.y))
			{
				if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				registerView_btnBack.OnMessage();// 返回登录界面
			}
		}
	}
	// 关闭绘图窗口
	closegraph();
}
void On_forgotPassView_btnConfirm_Click()
{
	string account(forgotPassView_txtName.Text());
	string email(forgotPassView_txtEmail.Text());
	forget_player.username = forgotPassView_txtName.Text();
	forget_player.email = forgotPassView_txtEmail.Text();

	//匹配不对
	if (!findEmByUsername(account, email))
	{
		MessageBox(GetHWnd(), "邮箱和账户不匹配", "错误", MB_OK);
		forgotPassView();
	}
	else
	{
		char s[100] = "您好，您的身份已核实，请稍等，尊敬的";
		strcat_s(s, sizeof(s), forgotPassView_txtName.Text());
		MessageBox(GetHWnd(), s, "请前往领取您的采矿证", MB_OK);
		resetPassView();
	}
}

void On_forgotPassView_btnReturn_Click()
{
	loginView();
}
bool findEmByUsername(string username, string email)
{
	Player players[100];
	ifstream ifs;
	ifs.open("userdata.txt");

	if (!ifs.is_open())
	{
		cout << "文件打开失败" << endl;
		return false;
	}

	string buf;
	int i = 0;

	//依次读出文件的内容按行
	while (getline(ifs, buf))
	{
		istringstream iss(buf);
		Player player;
		string scoreStr;

		// 读取每个字段并填充 Player 结构体
		if (getline(iss, player.username, ' ') &&
			getline(iss, player.password, ' ') &&
			getline(iss, player.email, ' ') &&
			getline(iss, player.name, ' ') &&
			getline(iss, scoreStr)
			)
		{
			// 将分数从字符串转换为整数
			player.score = stoi(scoreStr);

			// 将 Player 结构体存储到数组中
			players[i] = player;
			i++;
		}
		else
		{
			cout << "数据格式错误: " << buf << endl;
		}
	}
	ifs.close();
	for (int j = 0; j < i; j++)
	{
		cout << players[i].name << endl;;
	}

	for (int j = 0; j < i; j++)
	{
		if (players[j].username == username && players[j].email == email)
		{
			return true;
		}
	}

	return false;
}

void resetPassView()
{
	BeginBatchDraw();
	cleardevice();
	//设置窗口颜色
	setbkcolor(RGB(255, 255, 255));
	cleardevice();

	IMAGE login;
	loadimage(&login, "./assests/resetPass.png");
	putimage(0, 0, &login);

	settextcolor(RGB(239, 218, 187));
	setbkmode(TRANSPARENT);//文本填充色：透明
	settextstyle(100, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型字魂无外润黑体(商用需授权)
	outtextxy(WINDOW_WIDTH / 2 - textwidth("重置密码") / 2, 200 - textheight("重置密码") / 2, "重置密码");//标题文本

	//登录界面显示
	settextcolor(RGB(239, 218, 187));//文本颜色：黑色
	settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
	setbkmode(TRANSPARENT);
	//solidrectangle(250, 310, 630, 350);
	outtextxy(250, 315, "新密码");
	//solidrectangle(250, 370, 630, 410);
	outtextxy(250, 375, "确认密码");

	settextcolor(BLACK);
	resetPassView_btnReturn.Create(470, 490, 575, 530, "返回", On_resetPassView_btnReturn_Click);
	resetPassView_btnConfirm.Create(470, 430, 575, 470, "确认", On_resetPassView_btnConfirm_Click);    // 创建确认控件

	settextcolor(BLACK);//文本颜色：黑色
	settextstyle(20, 0, "字魂无外润黑体(商用需授权)");
	//solidrectangle(370, 315, 770, 345);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("登入") / 2, 480, "登入");
	txtNewpass.Create(370, 315, 700, 345, 20);

	//solidrectangle(370, 375, 770, 405);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("登入") / 2, 540, "注册");
	txtPass.Create(370, 375, 700, 405, 20);
	EndBatchDraw();
	while (true)
	{
		msg = getmessage(EX_MOUSE);            // 获取消息输入

		if (msg.message == WM_LBUTTONDOWN)
		{
			// 判断控件
			if (txtNewpass.Check(msg.x, msg.y))    txtNewpass.OnMessage();

			// 判断控件
			if (txtPass.Check(msg.x, msg.y))        txtPass.OnMessage();

			// 判断控件
			if (resetPassView_btnConfirm.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				resetPassView_btnConfirm.OnMessage();
			}

			if (resetPassView_btnReturn.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				resetPassView_btnReturn.OnMessage();
			}
		}
	}

	// 关闭绘图窗口
	closegraph();

	getchar();
}
void On_resetPassView_btnReturn_Click()
{
	forgotPassView();
}

void On_resetPassView_btnConfirm_Click()
{
	string pass(txtNewpass.Text());
	string newPass(txtPass.Text());
	if (pass != newPass)
	{
		MessageBox(GetHWnd(), "两次输入的密码不相同", "错误", MB_OK);
		resetPassView();
	}
	//匹配不对
	if (!findPlayByUsername(pass))
	{
		MessageBox(GetHWnd(), "重置错误", "错误", MB_OK);
		resetPassView();
	}
	else
	{
		loginView();
	}
}
bool findPlayByUsername(string password)
{
	Player players[100];
	bool found = false;
	ifstream ifs;
	ifs.open("userdata.txt");

	if (!ifs.is_open())
	{
		cout << "文件打开失败" << endl;
		return found;
	}

	string buf;
	int i = 0;

	//依次读出文件的内容按行
	while (getline(ifs, buf))
	{
		istringstream iss(buf);
		Player player;
		string scoreStr;

		// 读取每个字段并填充 Player 结构体
		if (getline(iss, player.username, ' ') &&
			getline(iss, player.password, ' ') &&
			getline(iss, player.email, ' ') &&
			getline(iss, player.name, ' ') &&
			getline(iss, scoreStr))
		{
			// 将分数从字符串转换为整数
			player.score = stoi(scoreStr);

			// 将 Player 结构体存储到数组中
			players[i] = player;
			i++;
		}
		else
		{
			cout << "数据格式错误: " << buf << endl;
		}
	}

	ifs.close();

	for (int j = 0; j < i; j++) {
		if (players[j].username == forget_player.username) {
			found = true;

			// 修改密码
			players[j].password = password;
			break;
		}
	}

	if (!found) {
		cout << "账号未找到" << endl;
		return found;
	}

	// 将修改后的数据写回到文件中
	ofstream ofs("userdata.txt");

	if (!ofs.is_open()) {
		cout << "文件打开失败" << endl;
		return found;
	}

	for (int j = 0; j < i; j++) {
		ofs << players[j].username << ' '
			<< players[j].password << ' '
			<< players[j].email << ' '
			<< players[j].name << ' '
			<< players[j].score << '\n';
	}

	ofs.close();
	return found;
}

void menuView()
{
	//播放主菜单音乐
	mciSendString("open gameMenu.mp3 alias musicMenu", NULL, 0, NULL);
	mciSendString("play musicMenu repeat", NULL, 0, NULL);
	if(systemMusicFlag == 1)mciSendString("setaudio musicMenu volume to 100", NULL, 0, NULL);
	//mciSendString("close musicMenu", NULL, 0, NULL);
	cleardevice();
	IMAGE main;
	loadimage(&main, "./assests/menu.png");
	while (true)
	{
		if (systemMusicFlag == 1) mciSendString("setaudio musicMenu volume to 100", NULL, 0, NULL);
		BeginBatchDraw();
		putimage(0, 0, &main);
		setbkcolor(BLACK);

		setlinecolor(BLACK);            // 设置画线颜色
		setbkcolor(WHITE);                // 设置背景颜色
		setfillcolor(RGB(248, 193, 90));            // 设置填充颜色

		settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(BLACK);
		fillrectangle(340, 200, 700, 240);
		outtextxy(470, 205, "开始游戏");
		fillrectangle(340, 270, 700, 310);
		outtextxy(470, 275, "游戏设置");
		fillrectangle(340, 340, 700, 380);
		outtextxy(470, 345, " 排行榜 ");
		fillrectangle(340, 410, 700, 450);
		outtextxy(470, 415, "更换皮肤");
		fillrectangle(340, 480, 700, 520);
		outtextxy(470, 485, "游戏说明");
		fillrectangle(340, 550, 700, 590);
		outtextxy(470, 555, "退出游戏");
		EndBatchDraw();
		if (peekmessage(&msg, EX_MOUSE) && msg.message == WM_LBUTTONDOWN)
		{
			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 200 && msg.y <= 240)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("setaudio musicMenu volume to 0", NULL, 0, NULL);

				gameView();//开始游戏
				cout << "点击鼠标右键1" << endl;
			}

			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 270 && msg.y <= 310)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				settingView();//游戏设置界面
				cout << "点击鼠标右键2" << endl;
			}

			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 340 && msg.y <= 380)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("setaudio musicMenu volume to 0", NULL, 0, NULL);
				rankView();//排行榜界面
				cout << "点击鼠标右键3" << endl;
			}

			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 410 && msg.y <= 450)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("setaudio musicMenu volume to 0", NULL, 0, NULL);
				skinView();//更换皮肤
				cout << "点击鼠标右键4" << endl;
			}

			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 480 && msg.y <= 520)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("setaudio musicMenu volume to 0", NULL, 0, NULL);
				explainView();//游戏说明
				cout << "点击鼠标右键5" << endl;
			}

			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 550 && msg.y <= 590)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("close musicMenu", NULL, 0, NULL);
				exitGame();// 退出游戏
				cout << "点击鼠标右键6" << endl;
			}
		}
	}
}
void rankView_On_btnEsc_Click() {
	menuView();
}
void rankView() {
	//播放排行榜音乐
	if (systemMusicFlag == 1) {
		mciSendString("open gameRank.mp3 alias musicRank", NULL, 0, NULL);
		mciSendString("play musicRank repeat", NULL, 0, NULL);
		mciSendString("setaudio musicRank volume to 100", NULL, 0, NULL);
	}
	setbkcolor(WHITE);
	// 声明一个map<int, vector<string>> rank vectoe<string>用于应对出现分数相同时只出现一个的情况
	map<int, vector<string>> rank;

	ifstream file(USERDATA_FILE);
	string line;
	while (getline(file, line)) {
		vector<string> data;
		stringstream ss(line);
		string token;
		while (getline(ss, token, ' ')) {
			data.push_back(token);
		}
		int score = stoi(data[4]);
		string name = data[3];
		rank[score].push_back(name);
	}
	file.close();

	cleardevice();
	IMAGE setting;
	loadimage(&setting, "./assests/rank.png");
	putimage(0, 0, &setting);

	/*settextstyle(100, 0, "字魂无外润黑体(商用需授权)");
	settextcolor(RGB(239, 218, 187));
	outtextxy(WINDOW_WIDTH / 2 - textwidth("排行榜") / 2, 50, "排行榜");*/

	int rankNum = 1;
	while (true) {
		rankNum = 1;
		// 展示排行榜
		int y = 200;
		settextcolor(RGB(239, 218, 187));
		outtextxy(320, y, "排名");
		outtextxy(450, y, "玩家");
		outtextxy(600, y, "分数");
		y += 50;
		for (auto it = rank.rbegin(); it != rank.rend(); ++it) {
			int score = it->first;
			for (const auto& name : it->second) {
				settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
				settextcolor(RGB(239, 218, 187));
				outtextxy(340, y, to_string(rankNum).c_str());
				outtextxy(450, y, name.c_str());
				outtextxy(600, y, to_string(score).c_str());
				y += 50;
				rankNum++;
				if (rankNum > 5) break;// 只展示前五名
			}
			if (rankNum > 5) break;
		}

		// 展示当前玩家信息
		settextstyle(20, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(RGB(239, 218, 187));
		outtextxy(330, 510, "当前玩家：");
		outtextxy(430, 510, currentUser.name.c_str());
		outtextxy(330, 550, "最高分数：");
		outtextxy(430, 550, to_string(currentUser.score).c_str()); // 这里需要根据currentUser获取当前玩家的分数

		// 创建并展示按钮
		settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(BLACK);
		setfillcolor(RGB(248, 193, 90));
		fillrectangle(900, 50, 1000, 100);
		outtextxy(920, 60, "返回");

		// 检测玩家鼠标输入
		ExMessage msg = getmessage(EX_MOUSE | EX_KEY);
		if (msg.message == WM_LBUTTONDOWN) {
			if (msg.x >= 900 && msg.x <= 1000 && msg.y >= 50 && msg.y <= 100) {
				if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("close musicRank", NULL, 0, NULL);
				cleardevice();
				break;
				//rankView_On_btnEsc_Click();
			}
		}
	}
}

void explainView()
{
	//播放游戏说明音乐
	if (systemMusicFlag == 1) {
		mciSendString("open gameExplain.mp3 alias musicExplain", NULL, 0, NULL);
		mciSendString("play musicExplain repeat", NULL, 0, NULL);
		mciSendString("setaudio musicExplain volume to 100", NULL, 0, NULL);
	}

	setbkcolor(WHITE);
	cleardevice();
	IMAGE setting;
	loadimage(&setting, "./assests/explain.png");
	putimage(0, 0, &setting);

	while (true)
	{
		settextstyle(50, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(RGB(239, 218, 187));
		outtextxy(230, 100, "游戏说明");
		settextstyle(25, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(RGB(239, 218, 187));
		outtextxy(230, 200, "键盘下键：出钩");
		outtextxy(230, 250, "ESC按键：暂停游戏");
		outtextxy(230, 300, "物品分数：钻石 > 黄金 > 石头");
		outtextxy(230, 350, "倒计时结束后：");
		outtextxy(230, 400, "到达目标分数可前往下一关");
		outtextxy(230, 450, "未达到目标分数游戏结束");

		settextstyle(50, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(RGB(239, 218, 187));
		outtextxy(600, 100, "制作人员");
		settextstyle(25, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(RGB(239, 218, 187));
		outtextxy(600, 200, "组长：阿鑫");
		outtextxy(600, 250, "副组长：阿熊");
		outtextxy(600, 300, "产品经理：阿魏 小滕");
		outtextxy(600, 350, "技术官：小常 约翰");
		outtextxy(600, 400, "监督官：超");
		outtextxy(600, 450, "信息官：阿源");

		//outtextxy(924, 50, "×");
		// 创建并展示按钮
		settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(BLACK);
		setfillcolor(RGB(248, 193, 90));
		fillrectangle(900, 50, 1000, 100);
		outtextxy(920, 60, "返回");

		ExMessage msg = getmessage(EX_MOUSE | EX_KEY);
		if (msg.message == WM_LBUTTONDOWN) {
			if (msg.x >= 900 && msg.x <= 1000 && msg.y >= 50 && msg.y <= 100) {
				if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("close musicExplain", NULL, 0, NULL);
				cleardevice();
				break;
				//rankView_On_btnEsc_Click();
			}
		}
	}
}

void settingView()
{
	setbkcolor(WHITE);
	cleardevice();
	BeginBatchDraw();

	IMAGE setting;
	loadimage(&setting, "./assests/setting.png");
	putimage(0, 0, &setting);

	//登录界面显示
	settextcolor(BLACK);//文本颜色：黑色
	settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
	setbkmode(TRANSPARENT);
	outtextxy(250, 238, "游戏音效(游戏外)");
	outtextxy(250, 335, "背景音乐(游戏外)");

	settextcolor(BLACK);//文本颜色：黑色
	btnGameOn.Create(560, 230, 660, 278, "开", On_btnGameOn_Click);
	btnGameOff.Create(670, 230, 770, 278, "关", On_btnGameOff_Click);
	btnBkOn.Create(560, 328, 660, 378, "开", On_btnBkOn_Click);
	btnBkOff.Create(670, 328, 770, 378, "关", On_btnBkOff_Click);
	btnEsc.Create(900, 50, 1000, 100, "返回", On_btnEsc_Click);

	EndBatchDraw();

	while (true)
	{
		msg = getmessage(EX_MOUSE);            // 获取消息输入

		if (msg.message == WM_LBUTTONDOWN)
		{
			// 判断控件
			if (btnGameOn.Check(msg.x, msg.y)) {
				PlaySound("./assests/musicswitch.wav", NULL, SND_FILENAME | SND_ASYNC);
				systemSoundFlag = 1;
				btnGameOn.OnMessage();
			}
			// 判断控件
			if (btnGameOff.Check(msg.x, msg.y)) {
				PlaySound("./assests/musicswitch.wav", NULL, SND_FILENAME | SND_ASYNC);
				systemSoundFlag = 0;
				btnGameOff.OnMessage();
			}
			// 判断控件
			if (btnBkOn.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/musicswitch.wav", NULL, SND_FILENAME | SND_ASYNC);
				systemMusicFlag = 1; 
				mciSendString("setaudio musicMenu volume to 100", NULL, 0, NULL);
				btnBkOn.OnMessage();
			}
			if (btnBkOff.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/musicswitch.wav", NULL, SND_FILENAME | SND_ASYNC);
				systemMusicFlag = 0;
				mciSendString("setaudio musicMenu volume to 0", NULL, 0, NULL);
				btnBkOff.OnMessage();
			}
			if (btnEsc.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				cleardevice();
				break;
				//btnEsc.OnMessage();
			}
		}
	}
}

void On_btnGameOn_Click()
{
	//mciSendString("setaudio 音频路径(全局变量) volume to 音量", NULL, 100, NULL);
	cout << "游戏音效开" << endl;
}

void On_btnGameOff_Click()
{
	//mciSendString("setaudio 音频路径(全局变量) volume to 音量", NULL, 0, NULL);
	cout << "游戏音效关" << endl;
}

void On_btnBkOn_Click()
{
	//mciSendString("setaudio 音频路径(全局变量) volume to 音量", NULL, 100, NULL);
	cout << "背景音效开" << endl;
}

void On_btnBkOff_Click()
{
	//mciSendString("setaudio 音频路径(全局变量) volume to 音量", NULL, 0, NULL);
	cout << "背景音效关" << endl;
}

void On_btnEsc_Click()
{
	menuView();
}
void button_yuanban_Click()
{
	//返回主菜单
	currentTheme = goldTheme;
	menuView();
}
void button_christmas_Click()
{
	//返回主菜单
	currentTheme = christmasTheme;
	menuView();
}
void button_apocalypseEventide_Click()
{
	//返回主菜单
	currentTheme = apocalypseTheme;
	menuView();
}
void button_maFestival_Click()
{
	//返回主菜单
	currentTheme = moonTheme;
	menuView();
}
void button_Halloween_Click()
{
	//返回主菜单
	currentTheme = halloweenTheme;
	menuView();
}
void button_changE_fishingPig()
{
	//返回主菜单
	currentTheme = changETheme;
	menuView();
}
void button_Menu_Click() {
	menuView();
}

void countDown(Time& countdown) {
	std::unique_lock<std::mutex> lck(mtx);

	// 等待开始信号
	cv.wait(lck, [] { return ready.load(); });

	while (true) {
		if (reset.load())
		{
			countdown.seconds = 60;
			reset.store(false);
		}
		if (stopFlag.load()) {
			std::cout << "线程暂停..." << std::endl;
			// 等待恢复信号
			cv.wait(lck, [] { return !stopFlag.load(); });
		}

		cout << "倒计时: " << countdown.seconds << " 秒" << std::endl;
		this_thread::sleep_for(std::chrono::seconds(1));
		countdown.seconds--;
	}

	std::cout << "时间到！" << std::endl;
}

void exitGame()
{
	closegraph();
	exit(0);
}

void skinView()
{
	//播放更换皮肤音频
	if (systemMusicFlag == 1) {
		mciSendString("open gameSkin.mp3 alias musicSkin", NULL, 0, NULL);
		mciSendString("play musicSkin repeat", NULL, 0, NULL);
		mciSendString("setaudio musicSkin volume to 100", NULL, 0, NULL);
	}

	cleardevice();
	BeginBatchDraw();

	IMAGE skin;
	loadimage(&skin, "./assests/skin.png");
	putimage(0, 0, &skin);

	//创建按钮对象
	EasyButton button1;
	EasyButton button2;
	EasyButton button3;
	EasyButton button4;
	EasyButton button5;
	EasyButton button6;
	EasyButton button7;
	//初始化更换皮肤界面矩形
	int skin_X = 0;
	int skin_Y = 0;
	int skin_W = WINDOW_WIDTH;
	int skin_H = WINDOW_HEIGHT;
	//初始化放置皮肤放置矩形
	int put_SkinX = 230;
	int put_SkinY = 160;
	int put_SkinW = 550;
	int put_SkinH = 380;
	setbkmode(TRANSPARENT);

	//绘制放置皮肤放置矩形
	setfillcolor(RGB(180, 180, 178));
	fillrectangle(put_SkinX, put_SkinY, put_SkinX + put_SkinW, put_SkinY + put_SkinH);
	//绘制默认皮肤放置
	IMAGE defaultIMG;
	loadimage(&defaultIMG, "./assests/goldTheme/switch.png", put_SkinW, put_SkinH);
	putimage(put_SkinX, put_SkinY, &defaultIMG);
	//绘制四个按钮
	settextcolor(BLACK);

	settextstyle(20, 0, "字魂无外润黑体(商用需授权)");

	int right_move = 200 - 10;
	int down_move = 300;
	int init_num = 315;

	button1.Create(120 + right_move, init_num + 250 + 20, 120 + 116 + right_move, init_num + 250 + 50 + 20, "黄金矿工", button_yuanban_Click);
	button2.Create(120 + 116 + 20 + right_move, init_num + 250 + 20, 120 + 232 + 20 + right_move, init_num + 250 + 50 + 20, "平安圣诞", button_christmas_Click);
	button3.Create(120 + 232 + 40 + right_move, init_num + 250 + 20, 120 + 348 + 40 + right_move, init_num + 250 + 50 + 20, "末日黄昏", button_apocalypseEventide_Click);
	button4.Create(120 + right_move, init_num + 50 + down_move, 120 + 116 + right_move, init_num + 100 + down_move, "花好月圆", button_maFestival_Click);
	button5.Create(120 + 116 + 20 + right_move, init_num + 50 + down_move, 120 + 232 + 20 + right_move, init_num + 100 + down_move, "万圣狂欢", button_Halloween_Click);
	button6.Create(120 + 232 + 40 + right_move, init_num + 50 + down_move, 120 + 348 + 40 + right_move, init_num + 100 + down_move, "嫦娥捞月", button_changE_fishingPig);

	settextstyle(20, 0, "字魂无外润黑体(商用需授权)");
	button7.Create(900, 50, 1000, 100, "返回", button_Menu_Click);

	//定义图片变量
	IMAGE yuanban_img;
	IMAGE charitsmas_img;
	IMAGE apocalypseEventide_img;
	IMAGE maFestival_img;
	IMAGE Halloween_img;
	IMAGE changE_fishingPig_img;
	//加载图片
	loadimage(&yuanban_img, "./assests/goldTheme/switch.png", put_SkinW, put_SkinH);
	//加载图片
	loadimage(&charitsmas_img, "./assests/christmasTheme/switch.png", put_SkinW, put_SkinH);
	//加载图片
	loadimage(&apocalypseEventide_img, "./assests/apocalypseTheme/switch.png", put_SkinW, put_SkinH);
	//加载图片
	loadimage(&maFestival_img, "./assests/moonTheme/switch.png", put_SkinW, put_SkinH);
	//加载图片
	loadimage(&Halloween_img, "./assests/halloweenTheme/switch.png", put_SkinW, put_SkinH);
	//加载图片
	loadimage(&changE_fishingPig_img, "./assests/changETheme/switch.png", put_SkinW, put_SkinH);
	EndBatchDraw();

	while (true)
	{
		msg = getmessage(EX_MOUSE);
		if (button1.Check(msg.x, msg.y))
		{
			//输出图片
			putimage(put_SkinX, put_SkinY, &yuanban_img);
			if (msg.message == WM_LBUTTONDOWN) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick2.wav", NULL, SND_FILENAME | SND_ASYNC);
				currentTheme = goldTheme;
				mciSendString("close musicSkin", NULL, 0, NULL);
				button1.OnMessage();
			}
		}

		if (button2.Check(msg.x, msg.y))
		{
			//输出图片
			putimage(put_SkinX, put_SkinY, &charitsmas_img);
			if (msg.message == WM_LBUTTONDOWN) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick2.wav", NULL, SND_FILENAME | SND_ASYNC);
				currentTheme = christmasTheme;
				mciSendString("close musicSkin", NULL, 0, NULL);
				button2.OnMessage();
			}
		}

		if (button3.Check(msg.x, msg.y))
		{
			//输出图片
			putimage(put_SkinX, put_SkinY, &apocalypseEventide_img);
			if (msg.message == WM_LBUTTONDOWN) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick2.wav", NULL, SND_FILENAME | SND_ASYNC);
				currentTheme = apocalypseTheme;
				mciSendString("close musicSkin", NULL, 0, NULL);
				button3.OnMessage();
			}
		}
		if (button4.Check(msg.x, msg.y))
		{
			//输出图片
			putimage(put_SkinX, put_SkinY, &maFestival_img);
			if (msg.message == WM_LBUTTONDOWN) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick2.wav", NULL, SND_FILENAME | SND_ASYNC);
				currentTheme = moonTheme;
				mciSendString("close musicSkin", NULL, 0, NULL);
				button4.OnMessage();
			}
		}
		if (button5.Check(msg.x, msg.y))
		{
			//输出图片
			putimage(put_SkinX, put_SkinY, &Halloween_img);
			if (msg.message == WM_LBUTTONDOWN) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick2.wav", NULL, SND_FILENAME | SND_ASYNC);
				currentTheme = halloweenTheme;
				mciSendString("close musicSkin", NULL, 0, NULL);
				button5.OnMessage();
			}
		}

		if (button6.Check(msg.x, msg.y))
		{
			//输出图片
			putimage(put_SkinX, put_SkinY, &changE_fishingPig_img);
			if (msg.message == WM_LBUTTONDOWN) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick2.wav", NULL, SND_FILENAME | SND_ASYNC);
				currentTheme = changETheme;
				mciSendString("close musicSkin", NULL, 0, NULL);
				button6.OnMessage();
			}
		}

		if (button7.Check(msg.x, msg.y) && msg.message == WM_LBUTTONDOWN)
		{
			if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
			mciSendString("close musicSkin", NULL, 0, NULL);
			cleardevice();
			break;
			//button7.OnMessage();
		}
	}
}
//----------------------------------------------------------界面--------------------------------------------------------------------

//---------------------------------------------------------游戏主体-----------------------------------------------------------------

void renderGame(map<string, LPCTSTR>& currentTheme, Hook& hook, Map& map, Time& countdown) {
	//声明变量
	IMAGE map_image, pause_image, person_image, countdown_image, hook_image;
	//渲染地图
	loadimage(&map_image, currentTheme.at("map"), WINDOW_WIDTH, WINDOW_HEIGHT);
	putimage(0, 0, &map_image);
	//渲染人物
	loadimage(&person_image, currentTheme.at("person"), 100, 100);
	drawImg((getwidth() - 100) / 2, 5, &person_image);
	//渲染倒计时
	setbkmode(TRANSPARENT);
	settextcolor(WHITE);
	settextstyle(20, 0, "字魂无外润黑体(商用需授权)");
	int currentTime = countdown.seconds;
	string currentTimeStr = to_string(currentTime);
	outtextxy(800, 60, "倒计时：");
	outtextxy(875, 60, currentTimeStr.c_str());
	//渲染游戏关卡
	int currentLevel = map.level;
	string currentLevelStr = to_string(currentLevel);
	outtextxy(800, 35, "当前关卡：");
	outtextxy(890, 35, currentLevelStr.c_str());
	//渲染目标分数
	int currentGoal = map.goal;
	string currentGoalStr = to_string(currentGoal);
	outtextxy(100, 35, "目标分数：");
	outtextxy(190, 35, currentGoalStr.c_str());
	//渲染当前分数
	int currentScore = map.accumulated_score;
	string currentScoreStr = to_string(currentScore);
	outtextxy(100, 60, "累计分数：");
	outtextxy(190, 60, currentScoreStr.c_str());
	//画绳子
	setlinecolor(BROWN);
	setlinestyle(PS_SOLID | PS_JOIN_BEVEL, 5);
	line(hook.x, hook.y, hook.position.first, hook.position.second);
	//渲染钩子
	//记得后期加上旋转
	//声明IMAGE变量 掩码图和旋转后的图片（原图、掩码图）
	IMAGE hookmask_image, rotatedHook_image, rotatedHookmask_image;

	if (hook.getitem.number == 0)//如果为空钩子
	{
		loadimage(&hookmask_image, currentTheme.at("hookmask"), hook.size, hook.size);
		loadimage(&hook_image, currentTheme.at("hook"), hook.size, hook.size);
		/*rotateimage(&rotatedHookmask_image, &hookmask_image, (PI / 180) * hook.angle, WHITE);
		rotateimage(&rotatedHook_image, &hook_image, (PI / 180) * hook.angle, BLACK);*/
		putimage(hook.position.first - hook.size / 2, hook.position.second, &hookmask_image, NOTSRCERASE);
		putimage(hook.position.first - hook.size / 2, hook.position.second, &hook_image, SRCINVERT);
	}
	else//不是空钩
	{
		switch (hook.getitem.number) {
		case 1:
			loadimage(&hookmask_image, currentTheme.at("goldhookmask"), hook.size, hook.size);
			loadimage(&hook_image, currentTheme.at("goldhook"), hook.size, hook.size);
			/*rotateimage(&rotatedHookmask_image, &hookmask_image, (PI/180)*hook.angle, WHITE);
			rotateimage(&rotatedHook_image, &hook_image, (PI / 180) * hook.angle, BLACK);*/
			putimage(hook.position.first - hook.size / 2, hook.position.second, &hookmask_image, NOTSRCERASE);
			putimage(hook.position.first - hook.size / 2, hook.position.second, &hook_image, SRCINVERT);
			break;
		case 2:
			loadimage(&hookmask_image, currentTheme.at("diamondhookmask"), hook.size, hook.size);
			loadimage(&hook_image, currentTheme.at("diamondhook"), hook.size, hook.size);
			/*rotateimage(&rotatedHookmask_image, &hookmask_image, hook.angle, WHITE);
			rotateimage(&rotatedHook_image, &hook_image, hook.angle, BLACK);*/
			putimage(hook.position.first - hook.size / 2, hook.position.second, &hookmask_image, NOTSRCERASE);
			putimage(hook.position.first - hook.size / 2, hook.position.second, &hook_image, SRCINVERT);
			break;
		case 3:
			loadimage(&hookmask_image, currentTheme.at("stonehookmask"), hook.size, hook.size);
			loadimage(&hook_image, currentTheme.at("stonehook"), hook.size, hook.size);
			/*rotateimage(&rotatedHookmask_image, &hookmask_image, hook.angle, WHITE);
			rotateimage(&rotatedHook_image, &hook_image, hook.angle, BLACK);*/
			putimage(hook.position.first - hook.size / 2, hook.position.second, &hookmask_image, NOTSRCERASE);
			putimage(hook.position.first - hook.size / 2, hook.position.second, &hook_image, SRCINVERT);
			break;
		}
	}

	//渲染物品
	for (auto& item : map.generated_Items) {
		IMAGE item_image;
		switch (item.number) {
		case 1:
			loadimage(&item_image, currentTheme.at("gold"), item.size, item.size);
			drawImg(item.position.first, item.position.second, &item_image);
			break;
		case 2:
			loadimage(&item_image, currentTheme.at("diamond"), item.size, item.size);
			drawImg(item.position.first, item.position.second, &item_image);
			break;
		case 3:
			loadimage(&item_image, currentTheme.at("stone"), item.size, item.size);
			drawImg(item.position.first, item.position.second, &item_image);
			break;
		}
	}
}

void initGame(bool& gameStatus, Time& countdown, Hook& hook, Map& map)
{	//游戏状态（true）
	gameStatus = true;
	//初始化倒计时（6000毫秒）
	countdown.seconds = 60;

	//初始化钩子
	hook.x = (getwidth() - 100) / 2 + 30;//初始化绳子的原始点
	hook.y = 95;
	hook.len = 50;
	hook.position = make_pair(hook.x, hook.y + hook.len);//原地位置
	hook.direction = 1;
	hook.getitem = {};
	hook.status = 0; // 摆动状态
	hook.angle = 0; // 垂直向下角度
	hook.speed = 7; // 默认速度
	//地图
	map.height = WINDOW_HEIGHT - 110; // 高度
	map.width = WINDOW_WIDTH; // 宽度
	map.goal = map.level * 1500; // 目标分数

	generateItems(map); // 调用generateItems()填充地图的随机物品数组
}

void generateItems(Map& map)
{	//物品大小
	//清除随机物品数组的元素
	map.generated_Items.clear();
	//随机数生成器
	random_device rd;
	mt19937 gen(rd());
	//生成物品的概率权重
	vector<double> weights(3);
	weights[0] = 0.4 - map.level * 0.01; // 黄金
	weights[1] = 0.2; // 钻石
	weights[2] = 0.4 + map.level * 0.01; // 石头
	discrete_distribution<int> dist(weights.begin(), weights.end());

	//生成位置
	auto generateRandomPosition = [&](int size)
		{int x, y;
	do
	{
		x = gen() % map.width;
		y = gen() % map.height + 110 + 100;
	} while (x + size > WINDOW_WIDTH || y + size > WINDOW_HEIGHT);     // 确保图片不超出地图范围
	return make_pair(x, y);
		};

	//调用isEmpty() 判断生成的物品坐标是否已被占用
	auto generateUniquePosition = [&](int size) -> pair<int, int>
		{	pair<int, int> pos;
	bool positionValid;
	do
	{
		pos = generateRandomPosition(size);
		positionValid = true;
		for (const auto& item : map.generated_Items)
		{
			if (!isEmpty(pos, item.position, size, item.size))
			{
				positionValid = false;
				break;
			}
		}
	} while (!positionValid);
	return pos;
		};

	//保底5金
	for (int i = 0; i < 5; i++)
	{
		Item item;
		item.number = 1;
		item.size = sizes[(gen() % 3) + 1];
		item.position = generateUniquePosition(item.size);
		item.score = getScore(item);
		map.generated_Items.push_back(item);
	}

	//随机10
	for (int i = 0; i < 10; i++)
	{
		Item item;
		int type = dist(gen) + 1;
		item.number = type;
		item.size = sizes[(gen() % 3) + 1];
		item.position = generateUniquePosition(item.size);
		item.score = getScore(item);

		map.generated_Items.push_back(item);
	}
}
void swingHook(Hook& hook)
{
	if (hook.status != 0) //出钩
	{
		return;
	}
	if (hook.direction == 1)
	{
		hook.angle += 1;
	}
	else if (hook.direction == -1)
	{
		hook.angle -= 1;
	}
	if (hook.angle >= 70)
	{
		hook.direction = -1;
	}
	if (hook.angle <= -70)
	{
		hook.direction = 1;
	}

	hook.position.first = hook.x + hook.len * sin(PI / 180 * hook.angle);//弧度和角度的转换
	hook.position.second = hook.y + hook.len * cos(PI / 180 * hook.angle);
}

bool isEmpty(pair<int, int> position1, pair<int, int> position2, int size1, int size2)
{
	vector<int> rec1 = { position1.first, position1.second, position1.first + size1, position1.second + size1 };
	vector<int> rec2 = { position2.first, position2.second, position2.first + size2, position2.second + size2 };

	// 判断矩形是否重叠
	if (rec1[0] >= rec2[2] || rec2[0] >= rec1[2])
	{
		return true; // 一方完全在另一方的右侧或左侧
	}
	if (rec1[1] >= rec2[3] || rec2[1] >= rec1[3])
	{
		return true; // 一方完全在另一方的上方或下方
	}

	// 如果上面的条件都不满足，说明矩形重叠
	return false;
}

int getScore(Item& item)
{
	return value[item.number] * item.size;
}

void pauseView(map<string, LPCTSTR>& currentTheme, bool& gameStatus, Map& map)
{
	/*string  soundOpenstr = "setaudio " + string(currentTheme.at("sound")) + "volume to 100";
	LPCTSTR soundOpen = soundOpenstr.c_str();
	string soundClosestr = "setaudio " + string(currentTheme.at("sound")) + "volume to 0";
	LPCTSTR soundClose = soundClosestr.c_str();
	string musicOpenstr = "setaudio " + string(currentTheme.at("music")) + "volume to 100";
	LPCTSTR musicOpen = musicOpenstr.c_str();
	string musicClosestr = "setaudio " + string(currentTheme.at("music")) + "volume to 0";
	LPCTSTR musicClose = musicClosestr.c_str();*/
	{
		//初始化暂停界面
		int pause_x = 312;
		int pause_y = 234;
		int pause_w = 400;
		int pause_h = 300;
		//初始化按钮1
		int button1_x = 20 + pause_x;
		int button1_y = 232 + pause_y;
		int button1_w = 150;
		int button1_h = 50;
		//初始化按钮2
		int button2_x = 230 + pause_x;
		int button2_y = 232 + pause_y;
		int button2_w = 150;
		int button2_h = 50;
		//初始化按钮3放游戏音效的开
		int button3_x = 220 + pause_x;
		int button3_y = 50 + pause_y;
		int button3_w = 45;
		int button3_h = 50;
		//初始化按钮4放背景音乐的开
		int button4_x = 220 + pause_x;
		int button4_y = 150 + pause_y;
		int button4_w = 45;
		int button4_h = 50;
		//初始化按钮5放游戏音效的关
		int button5_x = 285 + pause_x;
		int button5_y = 50 + pause_y;
		int button5_w = 45;
		int button5_h = 50;
		//初始化按钮6放背景音乐的关
		int button6_x = 285 + pause_x;
		int button6_y = 150 + pause_y;
		int button6_w = 45;
		int button6_h = 50;
		//初始化（继续游戏）文本
		int text_continueX = (button1_w - textwidth("继续游戏")) / 2 - 5;
		int text_continueY = (button1_h - textheight("继续游戏")) / 2 - 5;
		//初始化（返回菜单）文本
		int text_backX = (button2_w - textwidth("返回菜单")) / 2 - 5;
		int text_backY = (button2_h - textheight("返回菜单")) / 2 - 5;
		//初始化（游戏音效）文本
		int text_gameAudioX = pause_x + 116;
		int text_gameAudioY = pause_y + 66;
		//初始化（背景音乐）文本
		int text_bkAudioX = pause_x + 116;
		int text_bkAudioY = pause_y + 166;
		//初始化（开）文本1
		int text_open1X = (button3_w - textwidth("开")) / 2;
		int text_open1Y = (button3_h - textheight("开")) / 2;
		//初始化（开）文本2
		int text_open2X = (button5_w - textwidth("开")) / 2;
		int text_open2Y = (button4_h - textheight("开")) / 2;
		//初始化（关）文本1
		int text_close1X = (button5_w - textwidth("关")) / 2;
		int text_close1Y = (button5_h - textheight("关")) / 2;
		//初始化（关）文本2
		int text_close2X = (button5_w - textwidth("关")) / 2;
		int text_clsoe2y = (button5_h - textheight("关")) / 2;
		while (true)
		{
			if (peekmessage(&msg, EX_MOUSE)) {}
			setbkmode(TRANSPARENT);
			BeginBatchDraw();
			//cleardevice();

			//双缓冲区域，避免重复绘制
			//绘制暂停界面

			setlinecolor(RGB(254, 222, 0));
			setlinestyle(PS_SOLID, 2);

			settextstyle(20, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型字魂无外润黑体(商用需授权)

			setfillcolor(RGB(243, 230, 204));

			fillrectangle(pause_x, pause_y, pause_w + pause_x, pause_h + pause_y);
			//绘制按钮1
			if (msg.x > button1_x && msg.x<button1_x + button1_w && msg.y>button1_y && msg.y < button1_y + button1_h)
			{
				setfillcolor(RGB(254, 222, 0));
				fillroundrect(button1_x, button1_y, button1_x + button1_w, button1_y + button1_h, 20, 20);
				if (msg.message == WM_LBUTTONDOWN)
				{
					if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
					cout << "返回游戏已按下" << endl;
					stopFlag.store(false);
					ready.store(true);
					cv.notify_all();
					break;
				}
			}
			else
			{
				setfillcolor(RGB(242, 186, 91));
				fillroundrect(button1_x, button1_y, button1_x + button1_w, button1_y + button1_h, 20, 20);
			}
			//绘制（继续游戏）文本
			outtextxy(button1_x + text_continueX, button1_y + text_continueY, "返回游戏");
			//绘制按钮2
			if (msg.x > button2_x && msg.x<button2_x + button2_w && msg.y>button2_y && msg.y < button2_y + button2_h)
			{
				setfillcolor(RGB(254, 222, 0));
				fillroundrect(button2_x, button2_y, button2_x + button2_w, button2_y + button2_h, 20, 20);
				if (msg.message == WM_LBUTTONDOWN)
				{
					if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
					loseGame(gameStatus, map);
					break;
				}
			}
			else
			{
				setfillcolor(RGB(242, 186, 91));
				fillroundrect(button2_x, button2_y, button2_x + button2_w, button2_y + button2_h, 20, 20);
			}

			//绘制（返回菜单）文本
			settextcolor(BLACK);
			outtextxy(button2_x + text_backX, button2_y + text_backY, "返回菜单");
			//绘制（游戏音效）文本
			settextstyle(30, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型字魂无外润黑体(商用需授权)
			outtextxy(text_gameAudioX - 50, text_gameAudioY - 5, "游戏音效");
			//绘制背景音乐）文本
			outtextxy(text_bkAudioX - 50, text_bkAudioY - 5, "背景音乐");
			settextstyle(20, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型字魂无外润黑体(商用需授权)
			//绘制按钮3
			if (msg.x > button3_x && msg.x<button3_x + button3_w && msg.y>button3_y && msg.y < button3_y + button3_h)
			{
				setfillcolor(RGB(254, 222, 0));

				if (msg.message == WM_LBUTTONDOWN)
				{
					if (systemSoundFlag == 1)PlaySound("./assests/musicswitch.wav", NULL, SND_FILENAME | SND_ASYNC);
					gameSoundFlag = 1;
				}
			}
			else
			{
				setfillcolor(RGB(242, 186, 91));
			}
			fillroundrect(button3_x, button3_y, button3_x + button3_w, button3_y + button3_h, 20, 20);
			//绘制（开）文本1
			settextcolor(BLACK);
			outtextxy(button3_x + text_open1X, button3_y + text_open1Y, "开");
			//绘制按钮4
			if (msg.x > button4_x && msg.x<button4_x + button4_w && msg.y>button4_y && msg.y < button4_y + button4_h)
			{
				setfillcolor(RGB(254, 222, 0));
				if (msg.message == WM_LBUTTONDOWN)
				{
					if (systemSoundFlag == 1)PlaySound("./assests/musicswitch.wav", NULL, SND_FILENAME | SND_ASYNC);
					mciSendString("setaudio music volume to 100", NULL, 0, NULL);
				}
			}
			else
			{
				setfillcolor(RGB(242, 186, 91));
			}
			fillroundrect(button4_x, button4_y, button4_x + button4_w, button4_y + button4_h, 20, 20);
			//绘制（开）文本2
			settextcolor(BLACK);
			outtextxy(button4_x + text_open2X, button4_y + text_open2Y, "开");
			//绘制按钮5
			if (msg.x > button5_x && msg.x<button5_x + button5_w && msg.y>button5_y && msg.y < button5_y + button5_h)
			{
				setfillcolor(RGB(254, 222, 0));
				if (msg.message == WM_LBUTTONDOWN)
				{
					if (systemSoundFlag == 1)PlaySound("./assests/musicswitch.wav", NULL, SND_FILENAME | SND_ASYNC);
					gameSoundFlag = 0;
				}
			}
			else
			{
				setfillcolor(RGB(242, 186, 91));
			}
			fillroundrect(button5_x, button5_y, button5_x + button5_w, button5_y + button5_h, 20, 20);
			//绘制（关）文本1
			outtextxy(button5_x + text_close1X, button5_y + text_close1Y, "关");
			//绘制按钮6
			if (msg.x > button6_x && msg.x<button6_x + button6_w && msg.y>button6_y && msg.y < button6_y + button6_h)
			{
				setfillcolor(RGB(254, 222, 0));
				if (msg.message == WM_LBUTTONDOWN)
				{
					if (systemSoundFlag == 1)PlaySound("./assests/musicswitch.wav", NULL, SND_FILENAME | SND_ASYNC);
					mciSendString("setaudio music volume to 0", NULL, 0, NULL);
				}
			}
			else
			{
				setfillcolor(RGB(242, 186, 91));
			}
			fillroundrect(button6_x, button6_y, button6_x + button6_w, button6_y + button6_h, 20, 20);
			//绘制（关）文本2
			outtextxy(button6_x + text_close1X, button6_y + text_close1Y, "关");
			EndBatchDraw();
			//消息接受初始化，防止console多次接受鼠标信息
			msg.message = 0;
		}
	}
}
void launchHook(Time& countdown, Hook& hook, Map& map, bool& gameStatus)
{
	while (hook.status == 1)
	{
		//设置60FPS
		double frameTime = 1000 / 60;
		int startTime = clock();
		//钩子根据当前角度前进并更新钩子坐标
		hook.v.first = sin(PI / 180 * hook.angle) * hook.speed;
		hook.v.second = cos(PI / 180 * hook.angle) * hook.speed;
		hook.position.first += hook.v.first;
		hook.position.second += hook.v.second;
		BeginBatchDraw();
		renderGame(currentTheme, hook, map, countdown);
		EndBatchDraw();
		collideDetect(countdown, hook, map);
		int endTime = clock();
		/*countdown.miliseconds -= frameTime / 8;*/
		if (frameTime - (endTime - startTime) > 0)
			Sleep(frameTime - (endTime - startTime));
		if (countdown.seconds <= 0)
		{
			stopFlag.store(true);
			if (isWin(map) == true) {
				winGame(gameStatus, countdown, hook, map);
			}
			else {
				loseGame(gameStatus, map);
			}
		}
	}
}
void pullHook(Time& countdown, Hook& hook, Map& map, bool& gameStatus) {
	while (hook.status == 2) {//钩子状态为回缩
		double frameTime = 1000.0 / 60;//60帧所需的毫秒数
		int startTime = clock();//开始计时
		hook.v.first = sin(PI / 180 * hook.angle) * hook.speed;
		hook.v.second = cos(PI / 180 * hook.angle) * hook.speed;
		hook.position.first -= hook.v.first;
		hook.position.second -= hook.v.second;
		BeginBatchDraw();
		renderGame(currentTheme, hook, map, countdown);//调用renderGame() 渲染新游戏画面
		EndBatchDraw();
		if (sqrt(pow(hook.position.first - hook.x, 2) + pow(hook.position.second - hook.y, 2)) <= hook.len) {
			if (hook.getitem.number != 0)
			{
				map.accumulated_score += hook.score;//更新分数
			}
			hook.speed = 7;//更新速度
			hook.status = 0;//更新钩子状态为摆动
			//IMAGE hook_image;
			//loadimage(&hook_image, currentTheme.at("hook"));
			//putimage(550, 400, &hook_image);//更新钩子图片
		}
		int endTime = clock();//结束计时
		/*countdown.miliseconds -= frameTime / 8;*///确保不超过每帧所需时间
		if (frameTime - (endTime - startTime) > 0)
			Sleep(frameTime - (endTime - startTime));//确保游戏以60帧速度运行
		if (countdown.seconds <= 0)
		{
			stopFlag.store(true);
			if (isWin(map) == true)
				winGame(gameStatus, countdown, hook, map);
			else {
				loseGame(gameStatus, map);
			}
		}
	}
}

bool isWin(Map& map)
{
	if (map.accumulated_score >= map.goal)
	{
		return true;
	}
	else if (map.accumulated_score < map.goal)
	{
		return false;
	}
}
void updateHookSpeed(Hook hook, Item item) {
	//根据物品的种类和大小更新钩子回缩速度
	if (item.number == 1) {//黄金
		if (item.size == sizes[1]) {//小黄金
			hook.speed = 5;
		}
		else if (item.size == sizes[2]) {//中黄金
			hook.speed = 4;
		}
		else if (item.size == sizes[3]) {//大黄金
			hook.speed = 3;
		}
	}
	else if (item.number == 2) {//钻石
		if (item.size == sizes[1]) {//小钻石
			hook.speed = 6;
		}
		else if (item.size == sizes[2]) {//中钻石
			hook.speed = 5;
		}
		else if (item.size == sizes[3]) {//大钻石
			hook.speed = 4;
		}
	}
	else if (item.number == 3) {//石头
		if (item.size == sizes[1]) {//小石头
			hook.speed = 5;
		}
		else if (item.size == sizes[2]) {//中石头
			hook.speed = 4;
		}
		else if (item.size == sizes[3]) {//大石头
			hook.speed = 3;
		}
	}
}
bool detectCollision(const Hook& hook, const Item& item) {
	// 检测钩子与物品的碰撞
	double distance = sqrt(pow(hook.position.first - (item.position.first + item.size / 2), 2) +
		pow((hook.position.second + hook.size / 2) - (item.position.second + item.size / 2), 2));
	return distance < (hook.size + item.size) / 2.0;
}

void collideDetect(Time& countdown, Hook& hook, Map& map)
{
	// 检查钩子是否超出窗口边界
	if (hook.position.first < 0 || hook.position.first > WINDOW_WIDTH ||
		hook.position.second < 0 || hook.position.second > WINDOW_HEIGHT) {
		// 超出边界时回缩钩子
		hook.status = 2;
	}

	for (auto it = map.generated_Items.begin(); it != map.generated_Items.end(); ) {
		if (detectCollision(hook, *it)) {
			// 碰撞发生，更新钩子状态
			hook.status = 2; // 回缩状态

			// 记录分数
			hook.score = it->score;

			// 更新钩子的速度
			updateHookSpeed(hook, *it);

			//绑定物品
			hook.getitem = *it;

			// 移除物品
			it = map.generated_Items.erase(it);

			// 更新钩子图片（合并状态的更新逻辑视具体情况而定）
			// updateHookImage(hook); // 示例函数
			//缩回钩子
			break;
		}
		else {
			it++;
		}
	}
}

void loseGame(bool& gameStatus, Map& map)
{
	reset.store(true);
	ifstream file(USERDATA_FILE);
	vector<Player>players;
	Player currentPlayer;//获得的当前用户在原文件的信息
	bool foundPlayer = false;//是否找到该用户

	//判断用户是否存在的代码块：

	if (file.is_open())//判断文件是否打开
	{
		string line;//用于读取文件的字符串变量
		while (getline(file, line))//用于将每行信息存储到line中
		{
			istringstream iss(line);//解析文件中的用户信息到line
			Player player;//用于存储当前行用户信息
			iss >> player.username >> player.password >> player.email >> player.name >> player.score;//提取该行用户信息
			players.push_back(player);//将解析出来的用户信息存储到容器
			if (player.username == currentUser.username)//判断用户是否存在
			{
				currentPlayer = player;//将该行用户信息传给currentplayer便于后面对比最高值
				foundPlayer = true;//找到该用户，bool类型改为true
			}
		}
		file.close();
	}
	else
	{
		cerr << "不能够打开文件" << endl;//错误提示
		return;//直接返回函数
	}
	// 更新文件中当前玩家的最高分数的代码块：
	if (foundPlayer)
	{
		int highestScore = currentPlayer.score;
		if (map.accumulated_score > highestScore)
		{
			currentUser.score = map.accumulated_score;
			currentPlayer.score = map.accumulated_score;
			ofstream outFile(USERDATA_FILE);//打开文件
			if (outFile.is_open())//判断文件是否打开
			{
				for (const Player& player : players)//容器所有用户信息遍历
				{
					if (player.username == currentUser.username)//判断是否找到该用户
					{
						outFile << player.username << " " << player.password << " " << player.email << " " << player.name << " " << currentPlayer.score << endl;//更新用户信息，将最高分数更改写入文件
					}
					else
					{
						outFile << player.username << " " << player.password << " " << player.email << " " << player.name << " " << player.score << endl;//保持原用户信息写入文件
					}
				}
				outFile.close();//关闭文件
			}
			else
			{
				cerr << "不能够打开文件进行书写" << endl;//提示错误
			}
		}
	}
	else
	{
		cerr << "未找到该用户" << std::endl;//提示错误
	}
	countdown.seconds = 0;
	msg.message = 0;
	gameStatus = false;
	mciSendString("close music", NULL, 0, NULL);
	loseView(map);
}

void winGame(bool& gameStatus, Time& countdown, Hook& hook, Map& map)
{
	gameStatus = true;
	map.level++;
	initGame(gameStatus, countdown, hook, map);
	winView(map);
}

void winView(Map& map)
{
	if (gameSoundFlag == 1)PlaySound("./assests/win.wav", NULL, SND_FILENAME | SND_ASYNC);
	//使文本透明化
	setbkmode(TRANSPARENT);
	//初始化胜利界面
	int winView_x = 312;
	int winView_y = 234;
	int winView_w = 400;
	int winView_h = 300;
	//初始化文本（score）
	int score_x = (winView_w - textwidth("Score：99999")) / 2;
	int score_y = (winView_h - textheight("Score：99999")) / 2;
	char str[100] = "";
	sprintf_s(str, "目前累计分数: %d", map.accumulated_score);//累计分数没有初始化，编译器自动分配含有非法数据的空间
	char str_level[100] = "";
	sprintf_s(str_level, "正在前往关卡: %d", map.level);
	//初始化按钮
	int button_x = winView_x + 100;
	int button_y = winView_y + 300 - 45;
	int button_w = 200;
	int button_h = 50;
	//初始化（下一关）文本
	int text_x = (button_w - textwidth("下一关")) / 2;
	int text_y = (button_h - textheight("下一关")) / 2;
	IMAGE winbk;
	IMAGE win;
	loadimage(&winbk, "./assests/winbk.png");
	loadimage(&win, "./assests/win.png");
	while (true)
	{
		if (peekmessage(&msg, EX_MOUSE))
		{
		}
		//双缓冲措施
		BeginBatchDraw();

		putimage(0, 0, &winbk);
		drawImg(winView_x, winView_y - 50, &win);

		//绘制得分文本
		settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(RGB(243, 230, 204));
		outtextxy(winView_x + 100, winView_y + score_y + 25, str);
		outtextxy(winView_x + 100, winView_y + score_y + 60, str_level);

		setlinecolor(RGB(255, 215, 0));
		setlinestyle(PS_SOLID, 2);

		settextstyle(20, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型黑体
		settextcolor(BLACK);

		//绘制按钮矩形
		if (msg.x > button_x && msg.x< button_x + button_w && msg.y>button_y && msg.y < button_y + button_h)
		{
			setfillcolor(RGB(254, 222, 0));
			fillroundrect(button_x, button_y, button_x + button_w, button_y + button_h, 20, 20);
			if (msg.message == WM_LBUTTONDOWN)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				stopFlag.store(false);
				cv.notify_all();
				break;
			}
		}
		else
		{
			setfillcolor(RGB(243, 230, 204));
			fillroundrect(button_x, button_y, button_x + button_w, button_y + button_h, 20, 20);
		}
		//绘制（下一关）文本
		settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(BLACK);
		outtextxy(button_x + text_x - 22, button_y + text_y - 9, "继续通关");
		EndBatchDraw();
		//消息接受初始化，防止console多次接受鼠标信息
		msg.message = 0;
	}
}

void loseView(Map& map)
{
	if (gameSoundFlag == 1)PlaySound("./assests/lose.wav", NULL, SND_FILENAME | SND_ASYNC);
	//使文本透明化
	setbkmode(TRANSPARENT);
	//初始化胜利界面
	int winView_x = 312;
	int winView_y = 234;
	int winView_w = 400;
	int winView_h = 300;
	//初始化文本（score）
	int score_x = (winView_w - textwidth("Finally Socre：99999")) / 2;
	int score_y = (winView_h - textheight("Finally Socre：99999")) / 2;
	char str[100] = "";
	sprintf_s(str, "目前累计分数: %d", map.accumulated_score);//累计分数没有初始化，编译器自动分配含有非法数据的空间
	char str_level[100] = "";
	sprintf_s(str_level, "当前关卡: %d", map.level);//累计分数没有初始化，编译器自动分配含有非法数据的空间
	//初始化按钮
	int button_x = winView_x + 100;
	int button_y = winView_y + 300 - 45;
	int button_w = 200;
	int button_h = 50;
	//初始化（返回菜单）文本
	int text_x = (button_w - textwidth("返回菜单")) / 2;
	int text_y = (button_h - textheight("返回菜单")) / 2;
	IMAGE losebk;
	IMAGE lose;
	loadimage(&losebk, "./assests/losebk.png");
	loadimage(&lose, "./assests/lose.png");
	while (true)
	{
		if (peekmessage(&msg, EX_MOUSE))
		{
		}
		//双缓冲措施
		BeginBatchDraw();

		putimage(0, 0, &losebk);
		drawImg(winView_x, winView_y - 50, &lose);

		//绘制得分文本
		settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(WHITE);
		outtextxy(winView_x + 100, winView_y + score_y + 25, str);
		outtextxy(winView_x + 100, winView_y + score_y + 60, str_level);

		setlinecolor(BLACK);
		setlinestyle(PS_SOLID, 2);

		settextstyle(20, 0, "字魂无外润黑体(商用需授权)");//字体大小&类型黑体
		settextcolor(BLACK);
		//绘制按钮矩形
		if (msg.x > button_x && msg.x< button_x + button_w && msg.y>button_y && msg.y < button_y + button_h)
		{
			setfillcolor(DARKGRAY);
			fillroundrect(button_x, button_y, button_x + button_w, button_y + button_h, 20, 20);
			if (msg.message == WM_LBUTTONDOWN)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				menuView();
			}
		}
		else
		{
			setfillcolor(LIGHTGRAY);
			fillroundrect(button_x, button_y, button_x + button_w, button_y + button_h, 20, 20);
		}
		//绘制（返回菜单）文本
		settextstyle(30, 0, "字魂无外润黑体(商用需授权)");
		settextcolor(BLACK);
		outtextxy(button_x + text_x - 15, button_y + text_y - 9, "返回菜单");
		EndBatchDraw();
		//消息接受初始化，防止console多次接受鼠标信息
		msg.message = 0;
	}
}

void gameView()
{
	//声明变量（游戏状态、倒计时、钩子、地图、音效状态）
	bool gameStatus;
	Hook hook;
	Map map;

	//初始化地图的两个变量：关卡难度 = 1、累计分数 = 0
	map.level = 1;
	map.accumulated_score = 0;

	//调用initGame() 初始化游戏数据（游戏状态、倒计时、钩子、地图）
	initGame(gameStatus, countdown, hook, map);
	//加上游戏内音乐
	string gameMusicStrOpen = "open " + string(currentTheme.at("music")) + " alias music";
	LPCTSTR gameMusicLPCOpen = gameMusicStrOpen.c_str();
	mciSendString(gameMusicLPCOpen, NULL, 0, NULL);
	mciSendString("play music repeat", NULL, 0, NULL);
	mciSendString("setaudio music volume to 100", NULL, 0, NULL);
	//std::lock_guard<std::mutex> lck(mtx);
	stopFlag.store(false);
	ready.store(true);
	cv.notify_all();//通知计时线程开始
	//循环
	while (true)
	{
		BeginBatchDraw();

		//设置60FPS：
		//    double frameTime = 1000 / 60 （60帧所需的毫秒数）
		//    使用(int startTime = clock())开始计时
		//    调用renderGame() 渲染游戏界面
		//    调用swingHook() 钩子摆动
		//    使用(int endTime = clock()) 结束计时
		//    Time.second -= frameTime
		//    sleep(frameTime - (endTime - startTime))
		double frameTime = 1000 / 60;
		int startTime = clock();
		renderGame(currentTheme, hook, map, countdown);
		int endTime = clock();
		/*countdown.miliseconds -= frameTime / 8;*/
		if (frameTime - (endTime - startTime) > 0)
			Sleep(frameTime - (endTime - startTime));
		//检测玩家输入：
		//    下键：更新钩子状态（1.出钩） 调用launchHook() 发射钩子
		//    ESC：调用pauseView() 暂停游戏

		if (hook.status == 0)
		{
			swingHook(hook);

			if (peekmessage(&msg, EX_KEY))
			{
				if (msg.message == WM_KEYDOWN)
				{
					switch (msg.vkcode)
					{
					case VK_DOWN:
						//加上游戏内音效
						if (gameSoundFlag == 1) PlaySound("./assests/goldTheme/sound.wav", NULL, SND_FILENAME | SND_ASYNC);

						cout << "下键已按下" << endl;
						hook.status = 1;
						launchHook(countdown, hook, map, gameStatus);
						break;
					case VK_ESCAPE:
						cout << "退出键已按下" << endl;
						stopFlag.store(true);
						pauseView(currentTheme, gameStatus, map);
						break;
					}
				}
			}
		}

		//判断(钩子状态 == 2.回缩) 调用pullHook() 回缩钩子*/

		if (hook.status == 2)
		{
			pullHook(countdown, hook, map, gameStatus);
			hook.getitem = {};//钩子回缩之后，重置为空钩
		}

		//判断(倒计时 == 0)：
		//    判断(isWin() == true)：调用winGame()
		//    判断(isWin() == false)：调用loseGame()

		if (countdown.seconds <= 0)
		{
			stopFlag.store(true);
			if (isWin(map) == true) {
				winGame(gameStatus, countdown, hook, map);
			}
			else {
				loseGame(gameStatus, map);
			}
		}

		msg.message = 0;

		EndBatchDraw();
	}
}
//----------------------------------------------------游戏主体--------------------------------------------------------------------
//-----------------------------------------------------程序入口--------------------------------------------------------------------
int main() {
	//加载外部字体包
	AddFontResourceEx("font.ttf", FR_PRIVATE, NULL);
	initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
	setbkcolor(WHITE);
	thread countdownrThread(countDown, ref(countdown));//加入计时线程
	//默认音量100%
	waveOutSetVolume(0, 0xFFFFFFFF);
	cleardevice();
	loginView();
	//删除外部字体包
	RemoveFontResourceEx("font.ttf", FR_PRIVATE, NULL);
	system("pause");
}