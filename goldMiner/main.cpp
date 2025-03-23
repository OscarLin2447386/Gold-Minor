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
#define USERDATA_FILE "userdata.txt" // string fileName ��ÿ�и�ʽ���û��� ���� ���� �ǳ� ������
#define WINDOW_HEIGHT 768// ���ڸ߶�
#define WINDOW_WIDTH 1024//���ڿ��
#define PI 3.14159265359 //pai

//--------------------------------------�������------------------------------------------
/*
	bool gameStatus
	��Ϸ״̬: 0.ʧ�� 1.ʤ��
	��¼��ǰ��Ϸʤ��״̬
*/
bool gameStatus;
/*
	��Ϸ�� ��Ϸ��
	��Ч״̬��0.�ر� 1.��
	����״̬��0.�ر� 1.��
*/
int gameSoundFlag = 1;
int systemSoundFlag = 1;
int systemMusicFlag = 1;
/*
	Item�ṹ��:

		int number
		��Ʒ���:1.�ƽ� 2.��ʯ 3.ʯͷ

		pair<int, int> position
		��Ʒ����

		int size
		��Ʒ��С(��������):
		-��������Ʒ�ı߳�(����ռ�õ�ͼ�������Χ)
		-��������Ʒ�Ĵ�С(������Ʒ�ߴ� ��:��ƽ� С�ƽ� ��ʯͷ Сʯͷ С��ʯ��)
		-��������Ʒ������(���㹳��ץȡ������ٶ�)

		int score
		��Ʒ����: ץȡ����Ʒ��������÷���

		string image ��ƷͼƬ·��
*/
struct Item {
	int number;
	int size;
	pair<int, int> position;
	int score;
};

/*
	Map�ṹ�壺

		int level����ʼ Lv 1��;
		��ǰ�ؿ��Ѷ�
		-ÿ��ͨ�عؿ��Ѷ�++

		vector<Item> generated_Items
		��ͼ��Ʒ�б�:
		-��ǰ��ͼ������ɵ���Ʒ

		int height(568)
		��ͼ�߶�:
		-������Ʒ���ɷ�Χ

		int width(1024)
		��ͼ���:
		-������Ʒ���ɷ�Χ

		int goal
		Ŀ�����: goal = level * 1500
		-�ڵ���ʱ�������ж���Ϸ��Ӯ

		int accumulated_score
		��ǰ�ۼƷ���
		-�ڵ���ʱ�������ж���Ϸ��Ӯ

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
	Hook�ṹ�壺

		int size = 55
		���Ӵ�С������ץȡ��Χ

		int score;
		����ץ����Ʒ�ķ���

		pair<int, int> position
		�������꣺������Ʒ��ײ��⺯��

		double speed;
		���ӣ�Ĭ���ٶ� = 7�������ͻ��������º���ٶȣ����ٶ�
		60 FPS:  Ĭ���ٶ�7 ���ӳ��������±߽�ֻ��1352ms(1.352��)
		�����ٶ�vsʵ��ʱ���
			6=1577ms	5=1893ms	4=2366ms	3=3155ms	2=4733ms

		int len;
		���Ӱڶ�ʱ����ԭ�صĳ���

		int status
		����״̬��0.�ڶ� 1.���� 2.����

		int x, y;
		���ӵĳ�ʼ��

		int direction
		���Ӱڶ�����: ��1��˳ʱ�루-1����ʱ��

		double angle
		���ӵİڶ��Ƕ�

		pair<double, double> v
		���Ӱڶ��Ƕ�:
		-�����������ӵ��ƶ�����
		-��һ��ֵ����ÿ���ƶ���x������ x�����ʸ��ݽǶȵ���
		-�ڶ���ֵ����ÿ���ƶ���y������ y�����ʱ���1����

		Item getitem
		��ʾ��������Ʒ����ΪNULL���ʾ�չ�

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
	�û��˺�
	string password
	�û�����
	string name
	�û��ǳ�
	int score
	�û���õ���߷���
	string email
	�û�����

	currentUser
	�����û�����Ϣ
*/
struct Player {
	string username;//�˺�
	string password;//����
	string email;//����
	string name;//�ǳ�
	int score;//��߷���
}currentUser, forget_player;

/*
	��ֵ���� Ԫ���±��Ӧ��Ʒ��� ͨ�������±�Ԫ�صõ���Ӧ��Ʒ�ļ�ֵ
	���ڼ���������Ʒ�ķ���
	value[1] = 7���ƽ�
	value[2] = 10����ʯ��
	value[3] = 1��ʯͷ��
*/
const int value[4] = { 0,7,10,1 }; //ȫ�ֱ���

/*
	��С���� Ԫ���±��ӦС�д� ͨ�������±�Ԫ�صõ���Ӧ��Ʒ�Ĵ�С
	����������Ʒ�Ĵ�С
	sizes[1] = 20��С��
	sizes[2] = 45���У�
	sizes[3] = 90����
*/
const int sizes[4] = { 0,20,45,90 }; //ȫ�ֱ���
/*
	Time�ṹ��:
		double miliseconds ��ǰ����ʱ������
		string image ����ʱľ��ͼƬ·��
*/
struct Time {
	int seconds;
}countdown;
/*
����ExMessageȫ�ֱ����������û�����
*/
ExMessage msg = { 0 };

/*
	ԭ����Ϸ����
*/
const map <string, LPCTSTR> goldTheme{ //ȫ�ֱ���
	{"map", "./assests/goldTheme/background.png"},
	{"pause", "./assests/goldTheme/pause.png"},
	{"countdown", "./assests/goldTheme/δ֪"},
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
	�϶��̰˽���Ϸ����
*/
const map <string, LPCTSTR> changETheme{ //ȫ�ֱ���
	{"map", "./assests/changETheme/background.png"},
	{"pause", "./assests/changETheme/pause.png"},
	{"countdown", "./assests/changETheme/δ֪"},
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
	ĩ�ջƻ���Ϸ����
*/
const map <string, LPCTSTR> apocalypseTheme{ //ȫ�ֱ���
	{"map", "./assests/apocalypseTheme/background.png"},
	{"pause", "./assests/apocalypseTheme/pause.png"},
	{"countdown", "./assests/apocalypseTheme/δ֪"},
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
	�������Ϸ����
*/
const map <string, LPCTSTR> moonTheme{ //ȫ�ֱ���
	{"map", "./assests/moonTheme/background.png"},
	{"pause", "./assests/moonTheme/pause.png"},
	{"countdown", "./assests/moonTheme/δ֪"},
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
	��ʥ����Ϸ����
*/
const map <string, LPCTSTR> halloweenTheme{ //ȫ�ֱ���
	{"map", "./assests/halloweenTheme/background.png"},
	{"pause", "./assests/halloweenTheme/pause.png"},
	{"countdown", "./assests/halloweenTheme/δ֪"},
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
	ʥ������Ϸ����
*/
const map <string, LPCTSTR> christmasTheme{ //ȫ�ֱ���
	{"map", "./assests/christmasTheme/background.png"},
	{"pause", "./assests/christmasTheme/pause.png"},
	{"countdown", "./assests/christmasTheme/δ֪"},
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
	ѡ�����Ϸ����
	����Ϸ���ⱻ������currentTheme�´����Ӧ��Ϸ�����map
	ͨ�� currentTheme["hook"] ���ܷ��ʵ����ӵ�ǰ�����ͼƬ·��
*/
map <string, LPCTSTR> currentTheme = goldTheme;//ȫ�ֱ���
//--------------------------------------------------------------
/*
	��ť����
*/
//��¼����
EasyTextBox txtName;
EasyTextBox txtPwd;
EasyButton btnLogIn;
EasyButton btnRegister;
EasyButton btnForgetPwd;
//�����������
EasyTextBox forgotPassView_txtName;
EasyTextBox forgotPassView_txtEmail;
EasyButton forgotPassView_btnConfirm;
EasyButton forgotPassView_btnReturn;
//�����������
EasyTextBox txtNewpass;
EasyTextBox txtPass;
EasyButton resetPassView_btnConfirm;
EasyButton resetPassView_btnReturn;
//���ý���
EasyButton btnGameOn;
EasyButton btnGameOff;
EasyButton btnBkOn;
EasyButton btnBkOff;
EasyButton btnEsc;
//ע�����
EasyTextBox registerView_txtUsername;
EasyTextBox registerView_txtPlayername;
EasyTextBox registerView_txtEmail;
EasyTextBox registerView_txtPassword;
EasyTextBox registerView_txtConfirmPassword;
EasyButton registerView_btnConfirm;
EasyButton registerView_btnBack;
//���߳�
mutex mtx;//������
condition_variable cv;//��������
atomic<bool> ready(false);//��ʼ�ź�
atomic<bool> stopFlag(false);//ֹͣ�ź�
atomic<bool> reset(false);//����ʱ���ź�

//--------------------------------------�������------------------------------------------

//--------------------------------------view------------------------------------------

/*
	��¼����
	�����ˣ�����
	���ܣ�
		����һ����ʱ�ַ���string temp (����ָ��ָ�����ַ���)
		�������ַ������û���string username������string pass �����ڴ����û����룩
		ѭ��(true)��
			չʾ������û��� ����
			չʾ��������û��������루������*���棩
			չʾѡ�1.��¼ 2.�������� 3.ע���˺�
			����û����������룺
				����û�����temp = username
				�������루��ȡ�����ַ�����
					�û�����ÿ�μ�⵽���ַ�����ӵ��û����ַ���tempĩβ
					username = temp;
				��������temp = pass
				�������루��ȡ�����ַ�����
					���룺ÿ�μ�⵽���ַ�����ӵ������ַ���tempĩβ
					pass = temp;
				��¼��
					����fileName�ļ���ÿ�и�ʽ���û��� ���� ���� �ǳ� ������
					��ȡÿ������ ȷ���Ƿ����ƥ����û���������
						�ɹ������û����ַ���������currentUserȫ�ֱ��������ڵ���Ϸ����ʱ�����û�����¼��������ʾ��½�ɹ� ����ѭ��
						ʧ�ܣ���ʾ�û��������벻ƥ�� ���������� ����û����������ַ���
				�������룺����forgotPassView() ��ת�����������
				ע���˺ţ�����registerView() ��תע���˺Ž���
	������void
	�������ͣ�void
*/
void loginView();

/*
	�����������
	�����ˣ�xxx
	���ܣ�
		����һ����ʱ�ַ���string temp (����ָ��ָ�����ַ���)
		�������ַ������û���string username������string email �����ڴ����û����룩
		ѭ��(true)��
			չʾ������û��� ����
			չʾ��������û���������
			չʾѡ�� 1.ȷ�� 2.���ص�¼����
			���������������룺
				����û�����temp = username
				�������루��ȡ�����ַ�����
					�û�����ÿ�μ�⵽���ַ�����ӵ��û����ַ���tempĩβ
					username = temp;
				��������temp = email
				�������루��ȡ�����ַ�����
					���䣺ÿ�μ�⵽���ַ�����ӵ������ַ���tempĩβ
					email = temp;
				ȷ����
					����fileName�ļ���ÿ�и�ʽ���û��� ���� ���� �ǳ� ���� ��
					��ȡÿ�У��û��� ���䣩ȷ���Ƿ����ƥ����û���������
						�ɹ�������resetPassView() ��ת����������� ����ѭ�� ���ص�¼����
						ʧ�ܣ���ʾ�û������䲻ƥ�� ����������
				���ص�¼���棺����ѭ�� ���ص�¼����
	������void
	�������ͣ�void
*/
void forgotPassView();

/*
	�����������
	�����ˣ�xxx
	���ܣ�
		����һ����ʱ�ַ���string temp (����ָ��ָ�����ַ���)
		�������ַ���������string pass��ȷ������string checkpass �����ڴ����û����룩
		ѭ��(true)��
			չʾ��������� ȷ������
			չʾ������������ȷ�����루�ַ�*���棩
			չʾѡ�� 1.ȷ�� 2.���ص�¼����
			���������������룺
				��������temp = pass
				�������루��ȡ�����ַ�����
					���룺ÿ�μ�⵽���ַ�����ӵ������ַ���tempĩβ
					pass = temp;
				���ȷ�������temp = checkpass
				�������루��ȡ�����ַ�����
					ȷ�����룺ÿ�μ�⵽���ַ�����ӵ�ȷ�������ַ���tempĩβ
					checkpass = temp;
				ȷ�ϣ�
					��֤�����ʽ�Ƿ����Ҫ�����볤��6-16λ��������ĸ�����֣�
					��֤ȷ�������Ƿ�һ��
						�ɹ�����ʾ��������ɹ� ����fileName�ļ� ����ѭ�� ���ص�¼����
						ʧ�ܣ���ʾ�����ʽ����ȷ��ȷ�����벻һ�� ����������
				���ص�¼���棺����ѭ�� ���ص�¼����
	������void
	�������ͣ�void
*/

// ����ؼ�
void resetPassView();

/*
	ע���˺Ž���
	�����ˣ�xxx
	���ܣ�
		����һ����ʱ�ַ���string temp (����ָ��ָ�����ַ���)
		�������ַ������û���string username �ǳ�string player ����string email ����string pass ȷ������string checkpass �����ڴ����û�����)
		�����ַ���:����string score = 0 ��Ĭ����߷���=0��
		ѭ��(true)��
			չʾ������û��� �ǳ� ���� ���� ȷ������
			չʾ�������û��� �ǳ� ���� ���� ȷ�����루�����ȷ��������*���棩
			չʾѡ�1.ȷ�� 2.���ص�¼����
			���������������룺
				����û�����temp = username
				�������루��ȡ�����ַ�����
					�û�����ÿ�μ�⵽���ַ�����ӵ��û����ַ���tempĩβ
					username = temp;
				����ǳƿ�temp = player
				�������루��ȡ�����ַ�����
					�ǳƣ�ÿ�μ�⵽���ַ�����ӵ��ǳ��ַ���tempĩβ
					player = temp;
				��������temp = email
				�������루��ȡ�����ַ�����
					���䣺ÿ�μ�⵽���ַ�����ӵ������ַ���tempĩβ
					email = temp;
				��������temp = pass
				�������루��ȡ�����ַ�����
					���룺ÿ�μ�⵽���ַ�����ӵ������ַ���tempĩβ
					pass = temp;
				���ȷ�������temp = checkpass
				�������루��ȡ�����ַ�����
					ȷ�����룺ÿ�μ�⵽���ַ�����ӵ�ȷ�������ַ���tempĩβ
					checkpass = temp;
				ȷ�ϣ�
					����fileName�ļ���ÿ�и�ʽ���û��� ���� ���� �ǳ� ������
					�ж��Ƿ��Ѵ��ڣ��û��� ���� �ǳ�
					��֤�����ʽ�Ƿ����Ҫ�����볤��6-16λ��������ĸ�����֣�
					�ж������ȷ�������Ƿ�һ��
						�ɹ������û��� ���� ���� �ǳ� ��������fileName�ļ���ÿ�и�ʽ���û��� ���� ���� �ǳ� ������
							  ��ʾע��ɹ� ����ѭ�� ���ص�¼����
						ʧ�ܣ���ʾ�û������������ʽ����ȷ��ȷ�����벻һ�� ����������
				���ص�¼���棺����ѭ�� ���ص�¼����
	������void
	�������ͣ�void
*/
void registerView();

/*
	���˵�����
	�����ˣ���
	���ܣ�
		ѭ��(true)��
			չʾѡ�1.��ʼ��Ϸ 2.��Ϸ���� 3.���а� 4.����Ƥ�� 5.��Ϸ˵�� 6.�˳���Ϸ
			���������룺
				��ʼ��Ϸ������gameView() ��ת��Ϸ����
				��Ϸ���ã�����settingView() ��ת��Ϸ���ý���
				���а񣺵���rankView() ��ת���а����
				����Ƥ��������skinView() ��ת����Ƥ������
				��Ϸ˵��������explainView() ��ת��Ϸ˵������
				�˳���Ϸ������exitGame() �˳�����
	������void
	�������ͣ�void
*/
void menuView();

/*
	��Ϸ����
	�����ˣ�����
	���ܣ�
		ѭ��������Ϸ�������֣�������Ƶ��currentTheme["music"]��ֵ��
		ʵ������������Ϸ״̬gameStatus������ʱcountdown������hook����ͼmap��
		��ʼ����ͼ�������������ؿ��Ѷ� = 1���ۼƷ��� = 0
		����initGame() ��ʼ����Ϸ���ݣ���Ϸ״̬������ʱ�����ӡ���ͼ��
		ѭ��(��Ϸ״̬ == true)��
			����60FPS��
				double frameTime = 1000/60 ��60֡����ĺ�������
				ʹ��(int startTime = clock())��ʼ��ʱ
			����renderGame() ��Ⱦ��Ϸ����
			����swingHook() ���Ӱڶ�
			ʹ��(int endTime = clock()) ������ʱ
			Time.milisecond -= frameTime
			sleep(frameTime - (endTime - startTime))
			���������룺
				�¼������¹���״̬��1.������ ����launchHook() ���乳��
				 ESC������pauseView() ��ͣ��Ϸ
			�ж�(����״̬== 2.����) ����pullHook() ��������
			�ж�(����ʱ == 0)��
				�ж�(isWin() == true)������winGame()
				�ж�(isWin() == false)������loseGame()
	������void
	�������ͣ�void
*/
void gameView();

/*
	��Ⱦ��Ϸ���棨����eaxyX �� putimage() �� loadimage()��
	�����ˣ�С�� ��Ϸ���棨1024 * 768�� ������Ϸ����ȷ��ÿ��ͼƬ������
	���ܣ� (ͼƬ������map<string,LPCTSTR> currentTheme)
	����ͨ�� currentTheme["hook"] ���ܷ��ʵ���ǰ����Ĺ���ͼƬ·��
		��Ⱦ��ͼ
		��Ⱦ��ͣ��
		��Ⱦ����
		��Ⱦ����ʱ
		��Ⱦ���� (����ʹ�С�ڹ��ӽṹ����)
		��Ⱦ��Ʒ ������ʹ�С�ڵ�ͼ�ṹ���ڣ�
	������map<string, LPCTSTR>& currentTheme, Hook& hook, Map& map, Time& countdown
	�������ͣ�void
*/
void renderGame(map<string, LPCTSTR>& currentTheme, Hook& hook, Map& map, Time& countdown);
/*
	��ͣ����
	�����ˣ���κ
	���ܣ�ͨ�� currentTheme["sound"] ���ܷ��ʵ���ǰ�������Ч��Ƶ·��
		ѭ��(true)��
			չʾѡ�1.������Ϸ 2.���ز˵�
			������������룺
				��Ϸ��Ч(��Ϸ��)����/�� = ���� 0/100 :
					mciSendString("setaudio ��Ƶ·�� volume to ����", NULL, 0, NULL);
					mciSendString("setaudio ��Ƶ·�� volume to ����", NULL, 100, NULL);
				��������(��Ϸ��)����/�� = ���� 0/100��
					mciSendString("setaudio ��Ƶ·�� volume to ����", NULL, 0, NULL);
					mciSendString("setaudio ��Ƶ·�� volume to ����", NULL, 100, NULL);
				������Ϸ������ѭ��
				���ز˵�������loseGame() ����ѭ��
	������map<string, LPCTSTR>& currentTheme, bool& gameStatus, Map& map
	�������ͣ�void
*/
void pauseView(map<string, LPCTSTR>& currentTheme, bool& gameStatus, Map& map);

/*
	�ж���Ӯ
	�����ˣ���κ
	���ܣ�
		�ж���һ����Ƿ�ﵽҪ��
			ʤ������true
			ʧ�ܷ���false
	������Map &map
	�������ͣ�bool
*/
bool isWin(Map& map);
/*
	ʤ������
	�����ˣ���κ
	���ܣ�
		ѭ��(true)��
			չʾʤ������ (��ʾ��ǰ����)
			������������� �������һ�أ�
			����ѭ��
	������Map &map
	�������ͣ�void
*/
void winView(Map& map);

/*
	ʧ�ܽ���
	������: ��κ
	����:
		ѭ��(true)��
			չʾʧ�ܽ��� (��ʾ��ǰ����)
			�������������
			������ز˵�ʱ
			����ѭ��
	������Map &map
	�������ͣ�void
*/
void loseView(Map& map);

/*
	���ý���
	�����ˣ�����
	���ܣ�
		ѭ��(true)��
			չʾѡ�1.��Ч���� 2.���ֿ��� 3.���ز˵�
			������������룺
				��Ϸ��Ч(��Ϸ��)����/�� = ���� 0/100 :
					mciSendString("setaudio ��Ƶ·��(ȫ�ֱ���) volume to ����", NULL, 0, NULL);
					mciSendString("setaudio ��Ƶ·��(ȫ�ֱ���) volume to ����", NULL, 100, NULL);
				��������(��Ϸ��)����/�� = ���� 0/100��
					mciSendString("setaudio ��Ƶ·��(ȫ�ֱ���) volume to ����", NULL, 0, NULL);
					mciSendString("setaudio ��Ƶ·��(ȫ�ֱ���) volume to ����", NULL, 100, NULL);
				���ز˵�������ѭ�� �������˵�
	������void
	�������ͣ�void
*/
void settingView();

/*
	���а���� (string fileName ��ÿ�и�ʽ���û��� ���� ���� �ǳ� ������)
	�����ˣ�xxx
	���ܣ�
		����һ��map<int,string> rank
		����fileName�ļ������������ǳ�װ��rank (map�����Զ����ݼ�����)
		ѭ��(true)��
			չʾ���а�չʾ����ǰʮ�������Ϣ���ǳ� ������
			�ײ�չʾ��ǰ�����Ϣ���ǳ� ������
			������������룺
				���ز˵�������ѭ�� �������˵�
	������void
	�������ͣ�void
*/
void rankView();
/*
	����Ƥ������
	�����ˣ�xxx
	���ܣ�
		ѭ��(true)��
			չʾ��Ϸ����ͼƬ
			չʾѡ�1.�����Ϸ���ⰴť 2.���ز˵�
			������������룺
				���ⰴť�������û���ѡ�����currentTheme��ֵ
				���ز˵�������ѭ�� �������˵�
	������void
	�������ͣ�void
*/
void skinView();
/*
	��Ϸ˵��
	�����ˣ���
	���ܣ�
		ѭ��(true)��
			չʾ��Ϸ�淨
			չʾ��Ϸ����С����Ϣ
			������������룺
				���ز˵�������ѭ�� �������˵�
	������void
	�������ͣ�void
*/
void explainView();
//--------------------------------------view------------------------------------------

//--------------------------------------service------------------------------------------
/*
	��ʼ����Ϸ���ݣ���Ϸ״̬������ʱ�����ӡ���ͼ��������ʼ����ͼ�Ĺؿ��Ѷȡ��ۼƷ�����
	�����ˣ�����
	���ܣ�
		��Ϸ״̬��true��
		��ʼ������ʱ��6000���룩
		��ʼ�����ӣ�ԭ��λ�á��ڶ�״̬����ֱ���½Ƕȡ�Ĭ���ٶȣ�
		��ͼ��
			�߶ȡ����
			����generateItems() ����ͼ�������Ʒ����
			Ŀ����� = �ؿ��Ѷ� * 1500
	������bool &gameStatus, Time &countdown, Hook &hook, Map &map
	�������ͣ�void
*/
void initGame(bool& gameStatus, Time& countdown, Hook& hook, Map& map);

/*
	���������Ʒ
	�����ˣ�����
	���ܣ�
		��������Ʒ�����Ԫ��
		����5�����׻ƽ� + ����10�������Ʒ��1�ƽ� 2��ʯ 3ʯͷ��
			��Ʒ��ţ�
				�������һ��0~1֮��ĸ�������0.01~0.40����ƽ� 0.41~0.60��ʾ��ʯ 0.61~1.0��ʾʯͷ
				����level���ӣ��ƽ�ռ�ȼ��� ʯͷռ������
				�ƽ�ռ�ȼ��٣�- (level * 0.01)
				ʯͷռ�����ӣ�+ (level * 0.01)
				�ο����루��Ȩ���ѡ��{
						vector<double> weights = {0.4 - level * 0.01, 0.2, 0.4 + level * 0.01}; �ƽ� ��ʯ ʯͷ
						discrete_distribution<int> dist(weights.begin(), weights.end());
						random_device rd; ���������
						mt19937 gen(rd());
						int result = dist(gen)+1;  ���������Ʒ��� 1~3
				}
			��Ʒ��С��
				ƽ���������ɴ�С������±� (1С 2�� 3��)
				ͨ���±���ʴ�С����õ���Ӧ�Ĵ�С (size = sizes[�±�])
			��Ʒ���꣺
				ȷ�����ɵ������ڵ�ͼ��Χ��
				����isEmpty() �ж����ɵ���Ʒ�����Ƿ��ѱ�ռ��
			��Ʒ������
				����getScore() ������Ʒ����
		�����ɵ�15����Ʒ�����ͼ�е������Ʒ����
	������Map &map
	�������ͣ�void
*/
void generateItems(Map& map);

/*
	�ж���Ʒ�����Ƿ��ѱ�ռ��
	�����ˣ�����
	���ܣ�
		��ȡ������Ʒ����ʹ�С���ж��Ƿ���в����ص�
		˼·�ο��� https://blog.csdn.net/xikangsoon/article/details/80458591

	������pair<int,int> position1��pair<int,int> position2��int size1��int size2
	�������ͣ�bool
*/
bool isEmpty(pair<int, int> position1, pair<int, int> position2, int size1, int size2);

/*
	������Ʒ����
	�����ˣ�����
	���ܣ�
		���� = value[��Ʒ���] * size
	������Item &item
	�������ͣ�int
*/
int getScore(Item& item);

/*
	���Ӱڶ�
	�����ˣ�����
	���ܣ�
		�����жϹ��Ӱڶ�����(1, -1)
		�ڶ��Ƕ� = �ڶ�����*�ڶ�����
		����180�������ذڶ�
		ÿ�ε��ù��Ӱڶ��Ƕ�+1
		�ж��Ƿ��޸İڶ�����
	������Hook &hook
	�������ͣ�void
*/
void swingHook(Hook& hook);

/*
	���乳��
	�����ˣ�����
	���ܣ�
		ѭ��(����״̬ == 1.����)��
			����60FPS��
				double frameTime = 1000/60 ��60֡����ĺ�������
				ʹ��(int startTime = clock())��ʼ��ʱ
			���Ӹ��ݵ�ǰ�Ƕ�ǰ�������¹�������
			����renderGame() ��Ⱦ����Ϸ����
			����collideDetect() �����Ʒ��ײ
			ʹ��(int endTime = clock()) ������ʱ
			Time.miliseconds -= frameTime
			sleep(frameTime - (endTime - startTime))
			�ж�(����ʱ == 0)��
				�ж�(isWin() == true)������winGame()
				�ж�(isWin() == false)������loseGame()
	������Time &countdown, Hook &hook, Map &map,bool gameStatus
	�������ͣ�void
*/
void launchHook(Time& countdown, Hook& hook, Map& map, bool& gameStatus);

/*
	��ײ���
	�����ˣ�����
	���ܣ�
		��⹳���Ƿ���ײ
			��ײ:  ���¹���״̬��2.������
				   Item�������Ƴ���ץȡ����Ʒ
				   ����ȡ����Ʒ������¼��Hook��score����
				   ����updateHookSpeed() ���¹��ӵ��ٶȣ�������Ʒ����ʹ�С��
				   ���¹���ͼƬ�����Ӻ���Ʒ�ϲ���
	������Time &countdown, Hook &hook, Map &map
	�������ͣ�void
*/
void collideDetect(Time& countdown, Hook& hook, Map& map);

/*
	��������
	�����ˣ�С��
	���ܣ�
		ѭ��(����״̬ == 2.����)��
			����60FPS��
				double frameTime = 1000/60 ��60֡����ĺ�������
				ʹ��(int startTime = clock())��ʼ��ʱ
			���Ӹ��ݵ�ǰ�ǶȻ��������¹�������
			����renderGame() ��Ⱦ����Ϸ����
			�ж��Ƿ񵽴�ԭ��
				����ԭ�أ�
					������Ϸ����(map)��������Ʒ������ʹ�С��
					���¹����ٶȣ�Ĭ���ٶȣ�
					���¹���״̬��0.�ڶ���
					���¹���ͼƬ��ԭʼͼƬ��
			ʹ��(int endTime = clock()) ������ʱ
			Time.miliseconds -= frameTime
			sleep(frameTime - (endTime - startTime))
			�ж�(����ʱ == 0)��
				�ж�(isWin() == true)������winGame()
				�ж�(isWin() == false)������loseGame()
	������Time &countdown, Hook &hook, Map map,bool gameStatus
	�������ͣ�void
*/
void pullHook(Time& countdown, Hook& hook, Map& map, bool& gameStatus);

/*
	���¹����ٶȣ�Ĭ���ٶ� = 7��
	�����ˣ�С��
	���ܣ�
		������Ʒ������ʹ�С���¹��ӻ����ٶȣ�
			�ƽ�С-��-�󣩣��ٶȣ�5-4-3��
			��ʯ��С-��-�󣩣��ٶȣ�6-5-4��
			ʯͷ��С-��-�󣩣��ٶȣ�5-4-3��
	������Hook &hook��Item &item
	�������ͣ�void
*/
void updateHookSpeed(Hook hook, Item item);
/*
	��Ϸʤ�� (�� isWin() == true && countdown == 0 ʱ����)
	�����ˣ���
	���ܣ�
		������Ϸ״̬Ϊtrue
		�ؿ��Ѷ�++
		����initGame() ��ʼ����Ϸ���ݣ���Ϸ״̬������ʱ�����ӡ���ͼ�� ��ʼ��һ�ؿ�
		����winView() չʾ��Ϸʤ������
	������bool gameStatus, Time countdown, Hook hook, Map map
	�������ͣ�void
*/
void winGame(bool& gameStatus, Time& countdown, Hook& hook, Map& map);

/*
	��Ϸʧ�� (�� isWin() == false && countdown == 0 ʱ����)
	������: ��
	���ܣ�
		������Ϸ״̬Ϊfalse
		����fileName�ļ� �ҵ���ǰ��ҵ��������� ��ÿ�и�ʽ���û��� ���� ���� �ǳ� ������
		�жϵ�ǰ��ҵķ����Ƿ������������߷�����
			���ڣ����µ�ǰ�����߷�����ֵ
		չʾ��Ϸʧ�ܽ���(����loseView)
	������bool& gameStatus, Map map
	�������ͣ�
*/
void loseGame(bool& gameStatus, Map& map);

/*
	������: ����
	���ܣ�
		������ӦbtnLogIn��ť���ж����ݿ����Ƿ��и��û�
	������void
	�������ͣ�void

*/
void On_btnLogIn_Click();

/*

	������: ����
	���ܣ�
		�������ݿ⣬�����Ƿ���username��email��Ӧ�����з���true,���򷵻�false
	������string username, string email
	�������ͣ�bool
*/
bool findEmByUsername(string username, string email);
/*
	������: ����
	���ܣ�
		�������ݿ⣬ͨ���û��������Ҹ��û�
	������string password
	�������ͣ�bool
*/
bool findPlayByUsername(string password);
/*

	������: ����
	���ܣ�
		����������������ȷ�ϰ�ť
	������void
	�������ͣ�void
*/
void On_resetPassView_btnConfirm_Click();

/*

	������: ����
	���ܣ�
		������ӦbtnConfirm��ť,ȷ��֮������ж��Ƿ��и��û��͸�����
	������void
	�������ͣ�void

*/
// ��ť forgotPassView_btnConfirm �ĵ���¼�
void On_forgotPassView_btnConfirm_Click();
/*

	������: ����
	���ܣ�
		������ӦbtnReturn��ť,���ص�¼����
	������void
	�������ͣ�void

*/
void On_forgotPassView_btnReturn_Click();

/*

	������: ����
	���ܣ�
		������ӦresetPassView_btnReturn��ť,���������������
	������void
	�������ͣ�void

*/
void On_resetPassView_btnReturn_Click();

/*

	������: ����
	���ܣ�
		������Ч�����ֿ��ذ�ť
	������void
	�������ͣ�void

*/
void On_btnGameOn_Click();

void On_btnGameOff_Click();

void On_btnBkOn_Click();

void On_btnBkOff_Click();

/*

	������: ����
	���ܣ�
		�������ý���ķ��ذ�ť���������˵�ҳ��
	������void
	�������ͣ�void

*/
void On_btnEsc_Click();

/*
	ע��������Ӻ���
*/
void registerView_On_btnRegister_Click();

/*
	���߳�ʵ�ֵ���ʱ����
*/
void countDown(Time& countdown);

/*
	������Ϸ
	�����ˣ���
	���ܣ�
		�˳�����
	������void
	�������ͣ�void
*/
void exitGame();
//------------------------------------------------------------------����-------------------------------------------------------------
void loginView()
{
	//�������˵�����
	mciSendString("open gameLogin.mp3 alias musicLogin", NULL, 0, NULL);
	mciSendString("play musicLogin repeat", NULL, 0, NULL);
	mciSendString("setaudio musicLogin volume to 100", NULL, 0, NULL);

	cleardevice();
	BeginBatchDraw();
	IMAGE login;
	loadimage(&login, "./assests/login.png");
	putimage(0, 0, &login);

	//����

	settextcolor(RGB(239, 218, 187));
	setbkmode(TRANSPARENT);//�ı����ɫ��͸��
	settextstyle(100, 0, "�ֻ����������(��������Ȩ)");//�����С&�����ֻ����������(��������Ȩ)
	outtextxy(WINDOW_WIDTH / 2 - textwidth("�ƽ��") / 2, 200 - textheight("�ƽ��") / 2, "�ƽ��");//�����ı�

	//��¼������ʾ
	settextcolor(RGB(239, 218, 187));//�ı���ɫ����ɫ
	settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
	setbkmode(TRANSPARENT);
	//solidrectangle(300, 310, 680, 350);
	outtextxy(278, 315, "�û���");
	//solidrectangle(300, 370, 680, 410);
	outtextxy(300, 375, "����");

	settextcolor(BLACK);
	//setfillcolor(YELLOW);
	//solidrectangle(340, 480, 700, 520);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("����") / 2, 480, "����");
	btnLogIn.Create(470, 430, 575, 470, "��¼", On_btnLogIn_Click);

	//solidrectangle(340, 540, 700, 580);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("����") / 2, 540, "ע��");
	btnRegister.Create(380, 500, 510, 540, "ע���˺�", registerView);

	//setfillcolor(WHITE);
	//settextstyle(30, 0, "�ֻ����������(��������Ȩ)");//�����С&�����ֻ����������(��������Ȩ)
	//solidrectangle(620, 420, 740, 460);
	//outtextxy(640, 430, "��������");
	btnForgetPwd.Create(530, 500, 660, 540, "��������", forgotPassView);

	//�����
	//setfillcolor(WHITE);
	//fillrectangle(370, 315, 750, 345); //�˺������
	settextcolor(BLACK);
	settextstyle(20, 0, "�ֻ����������(��������Ȩ)");
	txtName.Create(370, 315, 700, 345, 20);
	//fillrectangle(370, 375, 750, 405); //���������
	txtPwd.Create(370, 375, 700, 405, 20);

	//�����ǰ����ͼ������
	//loadimage(Page[0], "�˺�ͼƬ·��");
	//putimage(0, 0, Page[0]);
	//loadimage(Page[0], "����ͼƬ·��");
	//putimage(0, 0, Page[0]);
	EndBatchDraw();

	while (true)
	{
		msg = getmessage(EX_MOUSE);            // ��ȡ��Ϣ����

		if (msg.message == WM_LBUTTONDOWN)
		{
			// �жϿؼ�
			if (btnLogIn.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				btnLogIn.OnMessage();
			}

			// �жϿؼ�
			if (btnRegister.Check(msg.x, msg.y)) {
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				btnRegister.OnMessage();
			}

			// �жϿؼ�
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
	string password(txtPwdText);//ǿתstring
	cout << "username:" << username << endl;
	cout << "password:" << password << endl;//����
	//----------------------------------------���ļ�-----------------------------------
	bool loginSuccess = false;

	FILE* fp;
	fopen_s(&fp, USERDATA_FILE, "r");
	if (fp == NULL) {
		cout << "�޷����ļ�!" << endl;
		return;
	}
	char line[256];
	while (fgets(line, sizeof(line), fp)) {
		string lineStr(line);
		// ȥ����ĩ�Ļ��з�
		lineStr.erase(lineStr.find_last_not_of("\r\n") + 1);

		// ����������
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

		// ��ȡ����
		string fileAccount = tokens[0];
		string filePassword = tokens[1];

		if (fileAccount == username && filePassword == password) {
			currentUser.username = tokens[0];
			currentUser.password = tokens[1];
			currentUser.email = tokens[2];
			currentUser.name = tokens[3];
			currentUser.score = str_int(tokens[4]);//����currentUser
			loginSuccess = true;
			break;
		}
	}

	fclose(fp);

	if (loginSuccess) {
		cout << "��¼�ɹ�!" << currentUser.name << endl;
		//����menuView()
		mciSendString("close musicLogin", NULL, 0, NULL);
		menuView();
	}
	else {
		//cout << "�û��������벻ƥ�䣬���������롣" << endl;
		MessageBox(GetHWnd(), "�û��������벻ƥ�䣬���������롣", "����", MB_OK);
		//��ս���
	}
}
void registerView_On_btnRegister_Click()
{
	// ��ȡ������е�����
	string username = registerView_txtUsername.Text();
	string playername = registerView_txtPlayername.Text();
	string email = registerView_txtEmail.Text();
	string password = registerView_txtPassword.Text();
	string confirmPassword = registerView_txtConfirmPassword.Text();

	// ����û������ǳơ����䡢�����ȷ������
	if (username.empty() || playername.empty() || email.empty() || password.empty() || confirmPassword.empty())
	{
		MessageBox(GetHWnd(), "�����������ֶ�", "����", MB_OK);
		return;
	}

	if (password != confirmPassword)
	{
		MessageBox(GetHWnd(), "�����ȷ�����벻һ��", "����", MB_OK);
		return;
	}

	// ��֤�����ʽ
	bool validPass = true;
	for (char c : password) {
		if (!isalnum(c)) {
			validPass = false;
			break;
		}
	}
	if (!validPass) {
		MessageBox(GetHWnd(), "�����ʽ����ȷ", "��ʾ", MB_OK);
	}
	// ��֤�����ʽ�Ƿ����Ҫ�����볤��6-16λ��������ĸ�����֣�
	//if (password.length() < 6 || password.length() > 16 || !isalnum(password))
	//{
	//	ShowMessage( "�����ʽ����ȷ");
	//	return;
	//}

	// ����fileName�ļ���ÿ�и�ʽ���û��� ���� ���� �ǳ� ������
	ifstream file(USERDATA_FILE);
	string line;
	while (getline(file, line))
	{
		// ����ÿ�����ݣ�������û����������Ƿ��Ѿ�����
		// ������ڣ���ʾ�û��û����������Ѿ�����
		string user, pass, mail, player, score;
		stringstream ss(line);
		ss >> user >> pass >> mail >> player >> score;
		if (user == username || mail == email || player == playername)
		{
			MessageBox(GetHWnd(), "�û�����������ǳ��Ѿ�����", "����", MB_OK);
			return;
		}
	}
	file.close();

	// ���û��� ���� ���� �ǳ� ��������fileName�ļ���ÿ�и�ʽ���û��� ���� ���� �ǳ� ������
	ofstream outfile(USERDATA_FILE, ios::app);
	outfile << username << " " << password << " " << email << " " << playername << " 0" << endl;
	outfile.close();

	// ��ʾע��ɹ�
	MessageBox(GetHWnd(), "ע��ɹ�", "��ʾ", MB_OK);

	// ����ѭ�� ���ص�¼����
	loginView();
}
void forgotPassView()
{
	BeginBatchDraw();
	cleardevice();
	//���ô�����ɫ
	setbkcolor(RGB(255, 255, 255));
	cleardevice();

	IMAGE login;
	loadimage(&login, "./assests/forgotPass.png");
	putimage(0, 0, &login);

	settextcolor(RGB(239, 218, 187));
	setbkmode(TRANSPARENT);//�ı����ɫ��͸��
	settextstyle(100, 0, "�ֻ����������(��������Ȩ)");//�����С&�����ֻ����������(��������Ȩ)
	outtextxy(WINDOW_WIDTH / 2 - textwidth("��������") / 2, 200 - textheight("��������") / 2, "��������");//�����ı�

	//��¼������ʾ
	settextcolor(RGB(239, 218, 187));//�ı���ɫ����ɫ
	settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
	setbkmode(TRANSPARENT);
	//solidrectangle(300, 310, 680, 350);
	outtextxy(278, 315, "�û���");
	//solidrectangle(300, 370, 680, 410);
	outtextxy(300, 375, "����");

	settextcolor(BLACK);
	forgotPassView_btnReturn.Create(470, 490, 575, 530, "����", On_forgotPassView_btnReturn_Click);
	forgotPassView_btnConfirm.Create(470, 430, 575, 470, "ȷ��", On_forgotPassView_btnConfirm_Click);    // ����ȷ�Ͽؼ�

	//setfillcolor(WHITE);
	settextstyle(20, 0, "�ֻ����������(��������Ȩ)");//�����С&�����ֻ����������(��������Ȩ)
	settextcolor(BLACK);
	//solidrectangle(370, 315, 770, 345);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("����") / 2, 480, "����");
	forgotPassView_txtName.Create(370, 315, 700, 345, 20);

	//solidrectangle(370, 375, 770, 405);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("����") / 2, 540, "ע��");
	forgotPassView_txtEmail.Create(370, 375, 700, 405, 20);
	EndBatchDraw();

	while (true)
	{
		msg = getmessage(EX_MOUSE);            // ��ȡ��Ϣ����

		if (msg.message == WM_LBUTTONDOWN)
		{
			// �жϿؼ�
			if (forgotPassView_txtName.Check(msg.x, msg.y))forgotPassView_txtName.OnMessage();

			// �жϿؼ�
			if (forgotPassView_txtEmail.Check(msg.x, msg.y))forgotPassView_txtEmail.OnMessage();

			// �жϿؼ�
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

	// �رջ�ͼ����
	closegraph();

	getchar();
}

void registerView() {
	BeginBatchDraw();
	cleardevice();
	//���ô�����ɫ
	setbkcolor(RGB(255, 255, 255));
	cleardevice();

	IMAGE login;
	loadimage(&login, "./assests/register.png");
	putimage(0, 0, &login);

	// ����
	settextcolor(RGB(239, 218, 187));
	setbkmode(TRANSPARENT); // �ı����ɫ��͸��
	settextstyle(100, 0, "�ֻ����������(��������Ȩ)"); // �����С&�����ֻ����������(��������Ȩ)
	outtextxy(WINDOW_WIDTH / 2 - textwidth("ע���˺�") / 2, 200 - textheight("ע���˺�") / 2, "ע���˺�"); // �����ı�

	// ע�������ʾ
	settextcolor(RGB(239, 218, 187)); // �ı���ɫ����ɫ
	settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
	setbkmode(TRANSPARENT);
	//solidrectangle(300, 310, 680, 350);
	outtextxy(278, 315, "�û���");
	//solidrectangle(300, 370, 680, 410);
	outtextxy(300, 375, "�ǳ�");
	//solidrectangle(300, 430, 680, 470);
	outtextxy(300, 435, "����");
	//solidrectangle(300, 490, 680, 530);
	outtextxy(300, 495, "����");
	//solidrectangle(300, 550, 680, 590);
	outtextxy(245, 555, "ȷ������");

	settextcolor(BLACK);
	registerView_btnConfirm.Create(470, 610, 575, 650, "ȷ��", registerView_On_btnRegister_Click);

	//setfillcolor(WHITE);

	//solidrectangle(620, 600, 740, 640);
	registerView_btnBack.Create(470, 670, 575, 710, "����", loginView);

	settextstyle(20, 0, "�ֻ����������(��������Ȩ)");//�����С&�����ֻ����������(��������Ȩ)
	settextcolor(BLACK);
	// �����
	registerView_txtUsername.Create(370, 315, 700, 345, 20);
	registerView_txtPlayername.Create(370, 375, 700, 405, 20);
	registerView_txtEmail.Create(370, 435, 700, 465, 20);
	registerView_txtPassword.Create(370, 495, 700, 525, 20);
	registerView_txtConfirmPassword.Create(370, 555, 700, 585, 20);

	EndBatchDraw();
	// ѭ��
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
				registerView_btnBack.OnMessage();// ���ص�¼����
			}
		}
	}
	// �رջ�ͼ����
	closegraph();
}
void On_forgotPassView_btnConfirm_Click()
{
	string account(forgotPassView_txtName.Text());
	string email(forgotPassView_txtEmail.Text());
	forget_player.username = forgotPassView_txtName.Text();
	forget_player.email = forgotPassView_txtEmail.Text();

	//ƥ�䲻��
	if (!findEmByUsername(account, email))
	{
		MessageBox(GetHWnd(), "������˻���ƥ��", "����", MB_OK);
		forgotPassView();
	}
	else
	{
		char s[100] = "���ã���������Ѻ�ʵ�����Եȣ��𾴵�";
		strcat_s(s, sizeof(s), forgotPassView_txtName.Text());
		MessageBox(GetHWnd(), s, "��ǰ����ȡ���Ĳɿ�֤", MB_OK);
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
		cout << "�ļ���ʧ��" << endl;
		return false;
	}

	string buf;
	int i = 0;

	//���ζ����ļ������ݰ���
	while (getline(ifs, buf))
	{
		istringstream iss(buf);
		Player player;
		string scoreStr;

		// ��ȡÿ���ֶβ���� Player �ṹ��
		if (getline(iss, player.username, ' ') &&
			getline(iss, player.password, ' ') &&
			getline(iss, player.email, ' ') &&
			getline(iss, player.name, ' ') &&
			getline(iss, scoreStr)
			)
		{
			// ���������ַ���ת��Ϊ����
			player.score = stoi(scoreStr);

			// �� Player �ṹ��洢��������
			players[i] = player;
			i++;
		}
		else
		{
			cout << "���ݸ�ʽ����: " << buf << endl;
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
	//���ô�����ɫ
	setbkcolor(RGB(255, 255, 255));
	cleardevice();

	IMAGE login;
	loadimage(&login, "./assests/resetPass.png");
	putimage(0, 0, &login);

	settextcolor(RGB(239, 218, 187));
	setbkmode(TRANSPARENT);//�ı����ɫ��͸��
	settextstyle(100, 0, "�ֻ����������(��������Ȩ)");//�����С&�����ֻ����������(��������Ȩ)
	outtextxy(WINDOW_WIDTH / 2 - textwidth("��������") / 2, 200 - textheight("��������") / 2, "��������");//�����ı�

	//��¼������ʾ
	settextcolor(RGB(239, 218, 187));//�ı���ɫ����ɫ
	settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
	setbkmode(TRANSPARENT);
	//solidrectangle(250, 310, 630, 350);
	outtextxy(250, 315, "������");
	//solidrectangle(250, 370, 630, 410);
	outtextxy(250, 375, "ȷ������");

	settextcolor(BLACK);
	resetPassView_btnReturn.Create(470, 490, 575, 530, "����", On_resetPassView_btnReturn_Click);
	resetPassView_btnConfirm.Create(470, 430, 575, 470, "ȷ��", On_resetPassView_btnConfirm_Click);    // ����ȷ�Ͽؼ�

	settextcolor(BLACK);//�ı���ɫ����ɫ
	settextstyle(20, 0, "�ֻ����������(��������Ȩ)");
	//solidrectangle(370, 315, 770, 345);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("����") / 2, 480, "����");
	txtNewpass.Create(370, 315, 700, 345, 20);

	//solidrectangle(370, 375, 770, 405);
	//outtextxy(WINDOW_WIDTH / 2 - textwidth("����") / 2, 540, "ע��");
	txtPass.Create(370, 375, 700, 405, 20);
	EndBatchDraw();
	while (true)
	{
		msg = getmessage(EX_MOUSE);            // ��ȡ��Ϣ����

		if (msg.message == WM_LBUTTONDOWN)
		{
			// �жϿؼ�
			if (txtNewpass.Check(msg.x, msg.y))    txtNewpass.OnMessage();

			// �жϿؼ�
			if (txtPass.Check(msg.x, msg.y))        txtPass.OnMessage();

			// �жϿؼ�
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

	// �رջ�ͼ����
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
		MessageBox(GetHWnd(), "������������벻��ͬ", "����", MB_OK);
		resetPassView();
	}
	//ƥ�䲻��
	if (!findPlayByUsername(pass))
	{
		MessageBox(GetHWnd(), "���ô���", "����", MB_OK);
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
		cout << "�ļ���ʧ��" << endl;
		return found;
	}

	string buf;
	int i = 0;

	//���ζ����ļ������ݰ���
	while (getline(ifs, buf))
	{
		istringstream iss(buf);
		Player player;
		string scoreStr;

		// ��ȡÿ���ֶβ���� Player �ṹ��
		if (getline(iss, player.username, ' ') &&
			getline(iss, player.password, ' ') &&
			getline(iss, player.email, ' ') &&
			getline(iss, player.name, ' ') &&
			getline(iss, scoreStr))
		{
			// ���������ַ���ת��Ϊ����
			player.score = stoi(scoreStr);

			// �� Player �ṹ��洢��������
			players[i] = player;
			i++;
		}
		else
		{
			cout << "���ݸ�ʽ����: " << buf << endl;
		}
	}

	ifs.close();

	for (int j = 0; j < i; j++) {
		if (players[j].username == forget_player.username) {
			found = true;

			// �޸�����
			players[j].password = password;
			break;
		}
	}

	if (!found) {
		cout << "�˺�δ�ҵ�" << endl;
		return found;
	}

	// ���޸ĺ������д�ص��ļ���
	ofstream ofs("userdata.txt");

	if (!ofs.is_open()) {
		cout << "�ļ���ʧ��" << endl;
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
	//�������˵�����
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

		setlinecolor(BLACK);            // ���û�����ɫ
		setbkcolor(WHITE);                // ���ñ�����ɫ
		setfillcolor(RGB(248, 193, 90));            // ���������ɫ

		settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(BLACK);
		fillrectangle(340, 200, 700, 240);
		outtextxy(470, 205, "��ʼ��Ϸ");
		fillrectangle(340, 270, 700, 310);
		outtextxy(470, 275, "��Ϸ����");
		fillrectangle(340, 340, 700, 380);
		outtextxy(470, 345, " ���а� ");
		fillrectangle(340, 410, 700, 450);
		outtextxy(470, 415, "����Ƥ��");
		fillrectangle(340, 480, 700, 520);
		outtextxy(470, 485, "��Ϸ˵��");
		fillrectangle(340, 550, 700, 590);
		outtextxy(470, 555, "�˳���Ϸ");
		EndBatchDraw();
		if (peekmessage(&msg, EX_MOUSE) && msg.message == WM_LBUTTONDOWN)
		{
			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 200 && msg.y <= 240)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("setaudio musicMenu volume to 0", NULL, 0, NULL);

				gameView();//��ʼ��Ϸ
				cout << "�������Ҽ�1" << endl;
			}

			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 270 && msg.y <= 310)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				settingView();//��Ϸ���ý���
				cout << "�������Ҽ�2" << endl;
			}

			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 340 && msg.y <= 380)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("setaudio musicMenu volume to 0", NULL, 0, NULL);
				rankView();//���а����
				cout << "�������Ҽ�3" << endl;
			}

			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 410 && msg.y <= 450)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("setaudio musicMenu volume to 0", NULL, 0, NULL);
				skinView();//����Ƥ��
				cout << "�������Ҽ�4" << endl;
			}

			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 480 && msg.y <= 520)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("setaudio musicMenu volume to 0", NULL, 0, NULL);
				explainView();//��Ϸ˵��
				cout << "�������Ҽ�5" << endl;
			}

			if (msg.x >= 340 && msg.x < 340 + 360 && msg.y >= 550 && msg.y <= 590)
			{
				if (systemSoundFlag == 1)PlaySound("./assests/lightclick.wav", NULL, SND_FILENAME | SND_ASYNC);
				mciSendString("close musicMenu", NULL, 0, NULL);
				exitGame();// �˳���Ϸ
				cout << "�������Ҽ�6" << endl;
			}
		}
	}
}
void rankView_On_btnEsc_Click() {
	menuView();
}
void rankView() {
	//�������а�����
	if (systemMusicFlag == 1) {
		mciSendString("open gameRank.mp3 alias musicRank", NULL, 0, NULL);
		mciSendString("play musicRank repeat", NULL, 0, NULL);
		mciSendString("setaudio musicRank volume to 100", NULL, 0, NULL);
	}
	setbkcolor(WHITE);
	// ����һ��map<int, vector<string>> rank vectoe<string>����Ӧ�Գ��ַ�����ͬʱֻ����һ�������
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

	/*settextstyle(100, 0, "�ֻ����������(��������Ȩ)");
	settextcolor(RGB(239, 218, 187));
	outtextxy(WINDOW_WIDTH / 2 - textwidth("���а�") / 2, 50, "���а�");*/

	int rankNum = 1;
	while (true) {
		rankNum = 1;
		// չʾ���а�
		int y = 200;
		settextcolor(RGB(239, 218, 187));
		outtextxy(320, y, "����");
		outtextxy(450, y, "���");
		outtextxy(600, y, "����");
		y += 50;
		for (auto it = rank.rbegin(); it != rank.rend(); ++it) {
			int score = it->first;
			for (const auto& name : it->second) {
				settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
				settextcolor(RGB(239, 218, 187));
				outtextxy(340, y, to_string(rankNum).c_str());
				outtextxy(450, y, name.c_str());
				outtextxy(600, y, to_string(score).c_str());
				y += 50;
				rankNum++;
				if (rankNum > 5) break;// ֻչʾǰ����
			}
			if (rankNum > 5) break;
		}

		// չʾ��ǰ�����Ϣ
		settextstyle(20, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(RGB(239, 218, 187));
		outtextxy(330, 510, "��ǰ��ң�");
		outtextxy(430, 510, currentUser.name.c_str());
		outtextxy(330, 550, "��߷�����");
		outtextxy(430, 550, to_string(currentUser.score).c_str()); // ������Ҫ����currentUser��ȡ��ǰ��ҵķ���

		// ������չʾ��ť
		settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(BLACK);
		setfillcolor(RGB(248, 193, 90));
		fillrectangle(900, 50, 1000, 100);
		outtextxy(920, 60, "����");

		// �������������
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
	//������Ϸ˵������
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
		settextstyle(50, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(RGB(239, 218, 187));
		outtextxy(230, 100, "��Ϸ˵��");
		settextstyle(25, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(RGB(239, 218, 187));
		outtextxy(230, 200, "�����¼�������");
		outtextxy(230, 250, "ESC��������ͣ��Ϸ");
		outtextxy(230, 300, "��Ʒ��������ʯ > �ƽ� > ʯͷ");
		outtextxy(230, 350, "����ʱ������");
		outtextxy(230, 400, "����Ŀ�������ǰ����һ��");
		outtextxy(230, 450, "δ�ﵽĿ�������Ϸ����");

		settextstyle(50, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(RGB(239, 218, 187));
		outtextxy(600, 100, "������Ա");
		settextstyle(25, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(RGB(239, 218, 187));
		outtextxy(600, 200, "�鳤������");
		outtextxy(600, 250, "���鳤������");
		outtextxy(600, 300, "��Ʒ������κ С��");
		outtextxy(600, 350, "�����٣�С�� Լ��");
		outtextxy(600, 400, "�ල�٣���");
		outtextxy(600, 450, "��Ϣ�٣���Դ");

		//outtextxy(924, 50, "��");
		// ������չʾ��ť
		settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(BLACK);
		setfillcolor(RGB(248, 193, 90));
		fillrectangle(900, 50, 1000, 100);
		outtextxy(920, 60, "����");

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

	//��¼������ʾ
	settextcolor(BLACK);//�ı���ɫ����ɫ
	settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
	setbkmode(TRANSPARENT);
	outtextxy(250, 238, "��Ϸ��Ч(��Ϸ��)");
	outtextxy(250, 335, "��������(��Ϸ��)");

	settextcolor(BLACK);//�ı���ɫ����ɫ
	btnGameOn.Create(560, 230, 660, 278, "��", On_btnGameOn_Click);
	btnGameOff.Create(670, 230, 770, 278, "��", On_btnGameOff_Click);
	btnBkOn.Create(560, 328, 660, 378, "��", On_btnBkOn_Click);
	btnBkOff.Create(670, 328, 770, 378, "��", On_btnBkOff_Click);
	btnEsc.Create(900, 50, 1000, 100, "����", On_btnEsc_Click);

	EndBatchDraw();

	while (true)
	{
		msg = getmessage(EX_MOUSE);            // ��ȡ��Ϣ����

		if (msg.message == WM_LBUTTONDOWN)
		{
			// �жϿؼ�
			if (btnGameOn.Check(msg.x, msg.y)) {
				PlaySound("./assests/musicswitch.wav", NULL, SND_FILENAME | SND_ASYNC);
				systemSoundFlag = 1;
				btnGameOn.OnMessage();
			}
			// �жϿؼ�
			if (btnGameOff.Check(msg.x, msg.y)) {
				PlaySound("./assests/musicswitch.wav", NULL, SND_FILENAME | SND_ASYNC);
				systemSoundFlag = 0;
				btnGameOff.OnMessage();
			}
			// �жϿؼ�
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
	//mciSendString("setaudio ��Ƶ·��(ȫ�ֱ���) volume to ����", NULL, 100, NULL);
	cout << "��Ϸ��Ч��" << endl;
}

void On_btnGameOff_Click()
{
	//mciSendString("setaudio ��Ƶ·��(ȫ�ֱ���) volume to ����", NULL, 0, NULL);
	cout << "��Ϸ��Ч��" << endl;
}

void On_btnBkOn_Click()
{
	//mciSendString("setaudio ��Ƶ·��(ȫ�ֱ���) volume to ����", NULL, 100, NULL);
	cout << "������Ч��" << endl;
}

void On_btnBkOff_Click()
{
	//mciSendString("setaudio ��Ƶ·��(ȫ�ֱ���) volume to ����", NULL, 0, NULL);
	cout << "������Ч��" << endl;
}

void On_btnEsc_Click()
{
	menuView();
}
void button_yuanban_Click()
{
	//�������˵�
	currentTheme = goldTheme;
	menuView();
}
void button_christmas_Click()
{
	//�������˵�
	currentTheme = christmasTheme;
	menuView();
}
void button_apocalypseEventide_Click()
{
	//�������˵�
	currentTheme = apocalypseTheme;
	menuView();
}
void button_maFestival_Click()
{
	//�������˵�
	currentTheme = moonTheme;
	menuView();
}
void button_Halloween_Click()
{
	//�������˵�
	currentTheme = halloweenTheme;
	menuView();
}
void button_changE_fishingPig()
{
	//�������˵�
	currentTheme = changETheme;
	menuView();
}
void button_Menu_Click() {
	menuView();
}

void countDown(Time& countdown) {
	std::unique_lock<std::mutex> lck(mtx);

	// �ȴ���ʼ�ź�
	cv.wait(lck, [] { return ready.load(); });

	while (true) {
		if (reset.load())
		{
			countdown.seconds = 60;
			reset.store(false);
		}
		if (stopFlag.load()) {
			std::cout << "�߳���ͣ..." << std::endl;
			// �ȴ��ָ��ź�
			cv.wait(lck, [] { return !stopFlag.load(); });
		}

		cout << "����ʱ: " << countdown.seconds << " ��" << std::endl;
		this_thread::sleep_for(std::chrono::seconds(1));
		countdown.seconds--;
	}

	std::cout << "ʱ�䵽��" << std::endl;
}

void exitGame()
{
	closegraph();
	exit(0);
}

void skinView()
{
	//���Ÿ���Ƥ����Ƶ
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

	//������ť����
	EasyButton button1;
	EasyButton button2;
	EasyButton button3;
	EasyButton button4;
	EasyButton button5;
	EasyButton button6;
	EasyButton button7;
	//��ʼ������Ƥ���������
	int skin_X = 0;
	int skin_Y = 0;
	int skin_W = WINDOW_WIDTH;
	int skin_H = WINDOW_HEIGHT;
	//��ʼ������Ƥ�����þ���
	int put_SkinX = 230;
	int put_SkinY = 160;
	int put_SkinW = 550;
	int put_SkinH = 380;
	setbkmode(TRANSPARENT);

	//���Ʒ���Ƥ�����þ���
	setfillcolor(RGB(180, 180, 178));
	fillrectangle(put_SkinX, put_SkinY, put_SkinX + put_SkinW, put_SkinY + put_SkinH);
	//����Ĭ��Ƥ������
	IMAGE defaultIMG;
	loadimage(&defaultIMG, "./assests/goldTheme/switch.png", put_SkinW, put_SkinH);
	putimage(put_SkinX, put_SkinY, &defaultIMG);
	//�����ĸ���ť
	settextcolor(BLACK);

	settextstyle(20, 0, "�ֻ����������(��������Ȩ)");

	int right_move = 200 - 10;
	int down_move = 300;
	int init_num = 315;

	button1.Create(120 + right_move, init_num + 250 + 20, 120 + 116 + right_move, init_num + 250 + 50 + 20, "�ƽ��", button_yuanban_Click);
	button2.Create(120 + 116 + 20 + right_move, init_num + 250 + 20, 120 + 232 + 20 + right_move, init_num + 250 + 50 + 20, "ƽ��ʥ��", button_christmas_Click);
	button3.Create(120 + 232 + 40 + right_move, init_num + 250 + 20, 120 + 348 + 40 + right_move, init_num + 250 + 50 + 20, "ĩ�ջƻ�", button_apocalypseEventide_Click);
	button4.Create(120 + right_move, init_num + 50 + down_move, 120 + 116 + right_move, init_num + 100 + down_move, "������Բ", button_maFestival_Click);
	button5.Create(120 + 116 + 20 + right_move, init_num + 50 + down_move, 120 + 232 + 20 + right_move, init_num + 100 + down_move, "��ʥ��", button_Halloween_Click);
	button6.Create(120 + 232 + 40 + right_move, init_num + 50 + down_move, 120 + 348 + 40 + right_move, init_num + 100 + down_move, "�϶�����", button_changE_fishingPig);

	settextstyle(20, 0, "�ֻ����������(��������Ȩ)");
	button7.Create(900, 50, 1000, 100, "����", button_Menu_Click);

	//����ͼƬ����
	IMAGE yuanban_img;
	IMAGE charitsmas_img;
	IMAGE apocalypseEventide_img;
	IMAGE maFestival_img;
	IMAGE Halloween_img;
	IMAGE changE_fishingPig_img;
	//����ͼƬ
	loadimage(&yuanban_img, "./assests/goldTheme/switch.png", put_SkinW, put_SkinH);
	//����ͼƬ
	loadimage(&charitsmas_img, "./assests/christmasTheme/switch.png", put_SkinW, put_SkinH);
	//����ͼƬ
	loadimage(&apocalypseEventide_img, "./assests/apocalypseTheme/switch.png", put_SkinW, put_SkinH);
	//����ͼƬ
	loadimage(&maFestival_img, "./assests/moonTheme/switch.png", put_SkinW, put_SkinH);
	//����ͼƬ
	loadimage(&Halloween_img, "./assests/halloweenTheme/switch.png", put_SkinW, put_SkinH);
	//����ͼƬ
	loadimage(&changE_fishingPig_img, "./assests/changETheme/switch.png", put_SkinW, put_SkinH);
	EndBatchDraw();

	while (true)
	{
		msg = getmessage(EX_MOUSE);
		if (button1.Check(msg.x, msg.y))
		{
			//���ͼƬ
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
			//���ͼƬ
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
			//���ͼƬ
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
			//���ͼƬ
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
			//���ͼƬ
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
			//���ͼƬ
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
//----------------------------------------------------------����--------------------------------------------------------------------

//---------------------------------------------------------��Ϸ����-----------------------------------------------------------------

void renderGame(map<string, LPCTSTR>& currentTheme, Hook& hook, Map& map, Time& countdown) {
	//��������
	IMAGE map_image, pause_image, person_image, countdown_image, hook_image;
	//��Ⱦ��ͼ
	loadimage(&map_image, currentTheme.at("map"), WINDOW_WIDTH, WINDOW_HEIGHT);
	putimage(0, 0, &map_image);
	//��Ⱦ����
	loadimage(&person_image, currentTheme.at("person"), 100, 100);
	drawImg((getwidth() - 100) / 2, 5, &person_image);
	//��Ⱦ����ʱ
	setbkmode(TRANSPARENT);
	settextcolor(WHITE);
	settextstyle(20, 0, "�ֻ����������(��������Ȩ)");
	int currentTime = countdown.seconds;
	string currentTimeStr = to_string(currentTime);
	outtextxy(800, 60, "����ʱ��");
	outtextxy(875, 60, currentTimeStr.c_str());
	//��Ⱦ��Ϸ�ؿ�
	int currentLevel = map.level;
	string currentLevelStr = to_string(currentLevel);
	outtextxy(800, 35, "��ǰ�ؿ���");
	outtextxy(890, 35, currentLevelStr.c_str());
	//��ȾĿ�����
	int currentGoal = map.goal;
	string currentGoalStr = to_string(currentGoal);
	outtextxy(100, 35, "Ŀ�������");
	outtextxy(190, 35, currentGoalStr.c_str());
	//��Ⱦ��ǰ����
	int currentScore = map.accumulated_score;
	string currentScoreStr = to_string(currentScore);
	outtextxy(100, 60, "�ۼƷ�����");
	outtextxy(190, 60, currentScoreStr.c_str());
	//������
	setlinecolor(BROWN);
	setlinestyle(PS_SOLID | PS_JOIN_BEVEL, 5);
	line(hook.x, hook.y, hook.position.first, hook.position.second);
	//��Ⱦ����
	//�ǵú��ڼ�����ת
	//����IMAGE���� ����ͼ����ת���ͼƬ��ԭͼ������ͼ��
	IMAGE hookmask_image, rotatedHook_image, rotatedHookmask_image;

	if (hook.getitem.number == 0)//���Ϊ�չ���
	{
		loadimage(&hookmask_image, currentTheme.at("hookmask"), hook.size, hook.size);
		loadimage(&hook_image, currentTheme.at("hook"), hook.size, hook.size);
		/*rotateimage(&rotatedHookmask_image, &hookmask_image, (PI / 180) * hook.angle, WHITE);
		rotateimage(&rotatedHook_image, &hook_image, (PI / 180) * hook.angle, BLACK);*/
		putimage(hook.position.first - hook.size / 2, hook.position.second, &hookmask_image, NOTSRCERASE);
		putimage(hook.position.first - hook.size / 2, hook.position.second, &hook_image, SRCINVERT);
	}
	else//���ǿչ�
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

	//��Ⱦ��Ʒ
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
{	//��Ϸ״̬��true��
	gameStatus = true;
	//��ʼ������ʱ��6000���룩
	countdown.seconds = 60;

	//��ʼ������
	hook.x = (getwidth() - 100) / 2 + 30;//��ʼ�����ӵ�ԭʼ��
	hook.y = 95;
	hook.len = 50;
	hook.position = make_pair(hook.x, hook.y + hook.len);//ԭ��λ��
	hook.direction = 1;
	hook.getitem = {};
	hook.status = 0; // �ڶ�״̬
	hook.angle = 0; // ��ֱ���½Ƕ�
	hook.speed = 7; // Ĭ���ٶ�
	//��ͼ
	map.height = WINDOW_HEIGHT - 110; // �߶�
	map.width = WINDOW_WIDTH; // ���
	map.goal = map.level * 1500; // Ŀ�����

	generateItems(map); // ����generateItems()����ͼ�������Ʒ����
}

void generateItems(Map& map)
{	//��Ʒ��С
	//��������Ʒ�����Ԫ��
	map.generated_Items.clear();
	//�����������
	random_device rd;
	mt19937 gen(rd());
	//������Ʒ�ĸ���Ȩ��
	vector<double> weights(3);
	weights[0] = 0.4 - map.level * 0.01; // �ƽ�
	weights[1] = 0.2; // ��ʯ
	weights[2] = 0.4 + map.level * 0.01; // ʯͷ
	discrete_distribution<int> dist(weights.begin(), weights.end());

	//����λ��
	auto generateRandomPosition = [&](int size)
		{int x, y;
	do
	{
		x = gen() % map.width;
		y = gen() % map.height + 110 + 100;
	} while (x + size > WINDOW_WIDTH || y + size > WINDOW_HEIGHT);     // ȷ��ͼƬ��������ͼ��Χ
	return make_pair(x, y);
		};

	//����isEmpty() �ж����ɵ���Ʒ�����Ƿ��ѱ�ռ��
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

	//����5��
	for (int i = 0; i < 5; i++)
	{
		Item item;
		item.number = 1;
		item.size = sizes[(gen() % 3) + 1];
		item.position = generateUniquePosition(item.size);
		item.score = getScore(item);
		map.generated_Items.push_back(item);
	}

	//���10
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
	if (hook.status != 0) //����
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

	hook.position.first = hook.x + hook.len * sin(PI / 180 * hook.angle);//���ȺͽǶȵ�ת��
	hook.position.second = hook.y + hook.len * cos(PI / 180 * hook.angle);
}

bool isEmpty(pair<int, int> position1, pair<int, int> position2, int size1, int size2)
{
	vector<int> rec1 = { position1.first, position1.second, position1.first + size1, position1.second + size1 };
	vector<int> rec2 = { position2.first, position2.second, position2.first + size2, position2.second + size2 };

	// �жϾ����Ƿ��ص�
	if (rec1[0] >= rec2[2] || rec2[0] >= rec1[2])
	{
		return true; // һ����ȫ����һ�����Ҳ�����
	}
	if (rec1[1] >= rec2[3] || rec2[1] >= rec1[3])
	{
		return true; // һ����ȫ����һ�����Ϸ����·�
	}

	// �������������������㣬˵�������ص�
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
		//��ʼ����ͣ����
		int pause_x = 312;
		int pause_y = 234;
		int pause_w = 400;
		int pause_h = 300;
		//��ʼ����ť1
		int button1_x = 20 + pause_x;
		int button1_y = 232 + pause_y;
		int button1_w = 150;
		int button1_h = 50;
		//��ʼ����ť2
		int button2_x = 230 + pause_x;
		int button2_y = 232 + pause_y;
		int button2_w = 150;
		int button2_h = 50;
		//��ʼ����ť3����Ϸ��Ч�Ŀ�
		int button3_x = 220 + pause_x;
		int button3_y = 50 + pause_y;
		int button3_w = 45;
		int button3_h = 50;
		//��ʼ����ť4�ű������ֵĿ�
		int button4_x = 220 + pause_x;
		int button4_y = 150 + pause_y;
		int button4_w = 45;
		int button4_h = 50;
		//��ʼ����ť5����Ϸ��Ч�Ĺ�
		int button5_x = 285 + pause_x;
		int button5_y = 50 + pause_y;
		int button5_w = 45;
		int button5_h = 50;
		//��ʼ����ť6�ű������ֵĹ�
		int button6_x = 285 + pause_x;
		int button6_y = 150 + pause_y;
		int button6_w = 45;
		int button6_h = 50;
		//��ʼ����������Ϸ���ı�
		int text_continueX = (button1_w - textwidth("������Ϸ")) / 2 - 5;
		int text_continueY = (button1_h - textheight("������Ϸ")) / 2 - 5;
		//��ʼ�������ز˵����ı�
		int text_backX = (button2_w - textwidth("���ز˵�")) / 2 - 5;
		int text_backY = (button2_h - textheight("���ز˵�")) / 2 - 5;
		//��ʼ������Ϸ��Ч���ı�
		int text_gameAudioX = pause_x + 116;
		int text_gameAudioY = pause_y + 66;
		//��ʼ�����������֣��ı�
		int text_bkAudioX = pause_x + 116;
		int text_bkAudioY = pause_y + 166;
		//��ʼ���������ı�1
		int text_open1X = (button3_w - textwidth("��")) / 2;
		int text_open1Y = (button3_h - textheight("��")) / 2;
		//��ʼ���������ı�2
		int text_open2X = (button5_w - textwidth("��")) / 2;
		int text_open2Y = (button4_h - textheight("��")) / 2;
		//��ʼ�����أ��ı�1
		int text_close1X = (button5_w - textwidth("��")) / 2;
		int text_close1Y = (button5_h - textheight("��")) / 2;
		//��ʼ�����أ��ı�2
		int text_close2X = (button5_w - textwidth("��")) / 2;
		int text_clsoe2y = (button5_h - textheight("��")) / 2;
		while (true)
		{
			if (peekmessage(&msg, EX_MOUSE)) {}
			setbkmode(TRANSPARENT);
			BeginBatchDraw();
			//cleardevice();

			//˫�������򣬱����ظ�����
			//������ͣ����

			setlinecolor(RGB(254, 222, 0));
			setlinestyle(PS_SOLID, 2);

			settextstyle(20, 0, "�ֻ����������(��������Ȩ)");//�����С&�����ֻ����������(��������Ȩ)

			setfillcolor(RGB(243, 230, 204));

			fillrectangle(pause_x, pause_y, pause_w + pause_x, pause_h + pause_y);
			//���ư�ť1
			if (msg.x > button1_x && msg.x<button1_x + button1_w && msg.y>button1_y && msg.y < button1_y + button1_h)
			{
				setfillcolor(RGB(254, 222, 0));
				fillroundrect(button1_x, button1_y, button1_x + button1_w, button1_y + button1_h, 20, 20);
				if (msg.message == WM_LBUTTONDOWN)
				{
					if (systemSoundFlag == 1)PlaySound("./assests/softclick.wav", NULL, SND_FILENAME | SND_ASYNC);
					cout << "������Ϸ�Ѱ���" << endl;
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
			//���ƣ�������Ϸ���ı�
			outtextxy(button1_x + text_continueX, button1_y + text_continueY, "������Ϸ");
			//���ư�ť2
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

			//���ƣ����ز˵����ı�
			settextcolor(BLACK);
			outtextxy(button2_x + text_backX, button2_y + text_backY, "���ز˵�");
			//���ƣ���Ϸ��Ч���ı�
			settextstyle(30, 0, "�ֻ����������(��������Ȩ)");//�����С&�����ֻ����������(��������Ȩ)
			outtextxy(text_gameAudioX - 50, text_gameAudioY - 5, "��Ϸ��Ч");
			//���Ʊ������֣��ı�
			outtextxy(text_bkAudioX - 50, text_bkAudioY - 5, "��������");
			settextstyle(20, 0, "�ֻ����������(��������Ȩ)");//�����С&�����ֻ����������(��������Ȩ)
			//���ư�ť3
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
			//���ƣ������ı�1
			settextcolor(BLACK);
			outtextxy(button3_x + text_open1X, button3_y + text_open1Y, "��");
			//���ư�ť4
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
			//���ƣ������ı�2
			settextcolor(BLACK);
			outtextxy(button4_x + text_open2X, button4_y + text_open2Y, "��");
			//���ư�ť5
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
			//���ƣ��أ��ı�1
			outtextxy(button5_x + text_close1X, button5_y + text_close1Y, "��");
			//���ư�ť6
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
			//���ƣ��أ��ı�2
			outtextxy(button6_x + text_close1X, button6_y + text_close1Y, "��");
			EndBatchDraw();
			//��Ϣ���ܳ�ʼ������ֹconsole��ν��������Ϣ
			msg.message = 0;
		}
	}
}
void launchHook(Time& countdown, Hook& hook, Map& map, bool& gameStatus)
{
	while (hook.status == 1)
	{
		//����60FPS
		double frameTime = 1000 / 60;
		int startTime = clock();
		//���Ӹ��ݵ�ǰ�Ƕ�ǰ�������¹�������
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
	while (hook.status == 2) {//����״̬Ϊ����
		double frameTime = 1000.0 / 60;//60֡����ĺ�����
		int startTime = clock();//��ʼ��ʱ
		hook.v.first = sin(PI / 180 * hook.angle) * hook.speed;
		hook.v.second = cos(PI / 180 * hook.angle) * hook.speed;
		hook.position.first -= hook.v.first;
		hook.position.second -= hook.v.second;
		BeginBatchDraw();
		renderGame(currentTheme, hook, map, countdown);//����renderGame() ��Ⱦ����Ϸ����
		EndBatchDraw();
		if (sqrt(pow(hook.position.first - hook.x, 2) + pow(hook.position.second - hook.y, 2)) <= hook.len) {
			if (hook.getitem.number != 0)
			{
				map.accumulated_score += hook.score;//���·���
			}
			hook.speed = 7;//�����ٶ�
			hook.status = 0;//���¹���״̬Ϊ�ڶ�
			//IMAGE hook_image;
			//loadimage(&hook_image, currentTheme.at("hook"));
			//putimage(550, 400, &hook_image);//���¹���ͼƬ
		}
		int endTime = clock();//������ʱ
		/*countdown.miliseconds -= frameTime / 8;*///ȷ��������ÿ֡����ʱ��
		if (frameTime - (endTime - startTime) > 0)
			Sleep(frameTime - (endTime - startTime));//ȷ����Ϸ��60֡�ٶ�����
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
	//������Ʒ������ʹ�С���¹��ӻ����ٶ�
	if (item.number == 1) {//�ƽ�
		if (item.size == sizes[1]) {//С�ƽ�
			hook.speed = 5;
		}
		else if (item.size == sizes[2]) {//�лƽ�
			hook.speed = 4;
		}
		else if (item.size == sizes[3]) {//��ƽ�
			hook.speed = 3;
		}
	}
	else if (item.number == 2) {//��ʯ
		if (item.size == sizes[1]) {//С��ʯ
			hook.speed = 6;
		}
		else if (item.size == sizes[2]) {//����ʯ
			hook.speed = 5;
		}
		else if (item.size == sizes[3]) {//����ʯ
			hook.speed = 4;
		}
	}
	else if (item.number == 3) {//ʯͷ
		if (item.size == sizes[1]) {//Сʯͷ
			hook.speed = 5;
		}
		else if (item.size == sizes[2]) {//��ʯͷ
			hook.speed = 4;
		}
		else if (item.size == sizes[3]) {//��ʯͷ
			hook.speed = 3;
		}
	}
}
bool detectCollision(const Hook& hook, const Item& item) {
	// ��⹳������Ʒ����ײ
	double distance = sqrt(pow(hook.position.first - (item.position.first + item.size / 2), 2) +
		pow((hook.position.second + hook.size / 2) - (item.position.second + item.size / 2), 2));
	return distance < (hook.size + item.size) / 2.0;
}

void collideDetect(Time& countdown, Hook& hook, Map& map)
{
	// ��鹳���Ƿ񳬳����ڱ߽�
	if (hook.position.first < 0 || hook.position.first > WINDOW_WIDTH ||
		hook.position.second < 0 || hook.position.second > WINDOW_HEIGHT) {
		// �����߽�ʱ��������
		hook.status = 2;
	}

	for (auto it = map.generated_Items.begin(); it != map.generated_Items.end(); ) {
		if (detectCollision(hook, *it)) {
			// ��ײ���������¹���״̬
			hook.status = 2; // ����״̬

			// ��¼����
			hook.score = it->score;

			// ���¹��ӵ��ٶ�
			updateHookSpeed(hook, *it);

			//����Ʒ
			hook.getitem = *it;

			// �Ƴ���Ʒ
			it = map.generated_Items.erase(it);

			// ���¹���ͼƬ���ϲ�״̬�ĸ����߼��Ӿ������������
			// updateHookImage(hook); // ʾ������
			//���ع���
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
	Player currentPlayer;//��õĵ�ǰ�û���ԭ�ļ�����Ϣ
	bool foundPlayer = false;//�Ƿ��ҵ����û�

	//�ж��û��Ƿ���ڵĴ���飺

	if (file.is_open())//�ж��ļ��Ƿ��
	{
		string line;//���ڶ�ȡ�ļ����ַ�������
		while (getline(file, line))//���ڽ�ÿ����Ϣ�洢��line��
		{
			istringstream iss(line);//�����ļ��е��û���Ϣ��line
			Player player;//���ڴ洢��ǰ���û���Ϣ
			iss >> player.username >> player.password >> player.email >> player.name >> player.score;//��ȡ�����û���Ϣ
			players.push_back(player);//�������������û���Ϣ�洢������
			if (player.username == currentUser.username)//�ж��û��Ƿ����
			{
				currentPlayer = player;//�������û���Ϣ����currentplayer���ں���Ա����ֵ
				foundPlayer = true;//�ҵ����û���bool���͸�Ϊtrue
			}
		}
		file.close();
	}
	else
	{
		cerr << "���ܹ����ļ�" << endl;//������ʾ
		return;//ֱ�ӷ��غ���
	}
	// �����ļ��е�ǰ��ҵ���߷����Ĵ���飺
	if (foundPlayer)
	{
		int highestScore = currentPlayer.score;
		if (map.accumulated_score > highestScore)
		{
			currentUser.score = map.accumulated_score;
			currentPlayer.score = map.accumulated_score;
			ofstream outFile(USERDATA_FILE);//���ļ�
			if (outFile.is_open())//�ж��ļ��Ƿ��
			{
				for (const Player& player : players)//���������û���Ϣ����
				{
					if (player.username == currentUser.username)//�ж��Ƿ��ҵ����û�
					{
						outFile << player.username << " " << player.password << " " << player.email << " " << player.name << " " << currentPlayer.score << endl;//�����û���Ϣ������߷�������д���ļ�
					}
					else
					{
						outFile << player.username << " " << player.password << " " << player.email << " " << player.name << " " << player.score << endl;//����ԭ�û���Ϣд���ļ�
					}
				}
				outFile.close();//�ر��ļ�
			}
			else
			{
				cerr << "���ܹ����ļ�������д" << endl;//��ʾ����
			}
		}
	}
	else
	{
		cerr << "δ�ҵ����û�" << std::endl;//��ʾ����
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
	//ʹ�ı�͸����
	setbkmode(TRANSPARENT);
	//��ʼ��ʤ������
	int winView_x = 312;
	int winView_y = 234;
	int winView_w = 400;
	int winView_h = 300;
	//��ʼ���ı���score��
	int score_x = (winView_w - textwidth("Score��99999")) / 2;
	int score_y = (winView_h - textheight("Score��99999")) / 2;
	char str[100] = "";
	sprintf_s(str, "Ŀǰ�ۼƷ���: %d", map.accumulated_score);//�ۼƷ���û�г�ʼ�����������Զ����京�зǷ����ݵĿռ�
	char str_level[100] = "";
	sprintf_s(str_level, "����ǰ���ؿ�: %d", map.level);
	//��ʼ����ť
	int button_x = winView_x + 100;
	int button_y = winView_y + 300 - 45;
	int button_w = 200;
	int button_h = 50;
	//��ʼ������һ�أ��ı�
	int text_x = (button_w - textwidth("��һ��")) / 2;
	int text_y = (button_h - textheight("��һ��")) / 2;
	IMAGE winbk;
	IMAGE win;
	loadimage(&winbk, "./assests/winbk.png");
	loadimage(&win, "./assests/win.png");
	while (true)
	{
		if (peekmessage(&msg, EX_MOUSE))
		{
		}
		//˫�����ʩ
		BeginBatchDraw();

		putimage(0, 0, &winbk);
		drawImg(winView_x, winView_y - 50, &win);

		//���Ƶ÷��ı�
		settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(RGB(243, 230, 204));
		outtextxy(winView_x + 100, winView_y + score_y + 25, str);
		outtextxy(winView_x + 100, winView_y + score_y + 60, str_level);

		setlinecolor(RGB(255, 215, 0));
		setlinestyle(PS_SOLID, 2);

		settextstyle(20, 0, "�ֻ����������(��������Ȩ)");//�����С&���ͺ���
		settextcolor(BLACK);

		//���ư�ť����
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
		//���ƣ���һ�أ��ı�
		settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(BLACK);
		outtextxy(button_x + text_x - 22, button_y + text_y - 9, "����ͨ��");
		EndBatchDraw();
		//��Ϣ���ܳ�ʼ������ֹconsole��ν��������Ϣ
		msg.message = 0;
	}
}

void loseView(Map& map)
{
	if (gameSoundFlag == 1)PlaySound("./assests/lose.wav", NULL, SND_FILENAME | SND_ASYNC);
	//ʹ�ı�͸����
	setbkmode(TRANSPARENT);
	//��ʼ��ʤ������
	int winView_x = 312;
	int winView_y = 234;
	int winView_w = 400;
	int winView_h = 300;
	//��ʼ���ı���score��
	int score_x = (winView_w - textwidth("Finally Socre��99999")) / 2;
	int score_y = (winView_h - textheight("Finally Socre��99999")) / 2;
	char str[100] = "";
	sprintf_s(str, "Ŀǰ�ۼƷ���: %d", map.accumulated_score);//�ۼƷ���û�г�ʼ�����������Զ����京�зǷ����ݵĿռ�
	char str_level[100] = "";
	sprintf_s(str_level, "��ǰ�ؿ�: %d", map.level);//�ۼƷ���û�г�ʼ�����������Զ����京�зǷ����ݵĿռ�
	//��ʼ����ť
	int button_x = winView_x + 100;
	int button_y = winView_y + 300 - 45;
	int button_w = 200;
	int button_h = 50;
	//��ʼ�������ز˵����ı�
	int text_x = (button_w - textwidth("���ز˵�")) / 2;
	int text_y = (button_h - textheight("���ز˵�")) / 2;
	IMAGE losebk;
	IMAGE lose;
	loadimage(&losebk, "./assests/losebk.png");
	loadimage(&lose, "./assests/lose.png");
	while (true)
	{
		if (peekmessage(&msg, EX_MOUSE))
		{
		}
		//˫�����ʩ
		BeginBatchDraw();

		putimage(0, 0, &losebk);
		drawImg(winView_x, winView_y - 50, &lose);

		//���Ƶ÷��ı�
		settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(WHITE);
		outtextxy(winView_x + 100, winView_y + score_y + 25, str);
		outtextxy(winView_x + 100, winView_y + score_y + 60, str_level);

		setlinecolor(BLACK);
		setlinestyle(PS_SOLID, 2);

		settextstyle(20, 0, "�ֻ����������(��������Ȩ)");//�����С&���ͺ���
		settextcolor(BLACK);
		//���ư�ť����
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
		//���ƣ����ز˵����ı�
		settextstyle(30, 0, "�ֻ����������(��������Ȩ)");
		settextcolor(BLACK);
		outtextxy(button_x + text_x - 15, button_y + text_y - 9, "���ز˵�");
		EndBatchDraw();
		//��Ϣ���ܳ�ʼ������ֹconsole��ν��������Ϣ
		msg.message = 0;
	}
}

void gameView()
{
	//������������Ϸ״̬������ʱ�����ӡ���ͼ����Ч״̬��
	bool gameStatus;
	Hook hook;
	Map map;

	//��ʼ����ͼ�������������ؿ��Ѷ� = 1���ۼƷ��� = 0
	map.level = 1;
	map.accumulated_score = 0;

	//����initGame() ��ʼ����Ϸ���ݣ���Ϸ״̬������ʱ�����ӡ���ͼ��
	initGame(gameStatus, countdown, hook, map);
	//������Ϸ������
	string gameMusicStrOpen = "open " + string(currentTheme.at("music")) + " alias music";
	LPCTSTR gameMusicLPCOpen = gameMusicStrOpen.c_str();
	mciSendString(gameMusicLPCOpen, NULL, 0, NULL);
	mciSendString("play music repeat", NULL, 0, NULL);
	mciSendString("setaudio music volume to 100", NULL, 0, NULL);
	//std::lock_guard<std::mutex> lck(mtx);
	stopFlag.store(false);
	ready.store(true);
	cv.notify_all();//֪ͨ��ʱ�߳̿�ʼ
	//ѭ��
	while (true)
	{
		BeginBatchDraw();

		//����60FPS��
		//    double frameTime = 1000 / 60 ��60֡����ĺ�������
		//    ʹ��(int startTime = clock())��ʼ��ʱ
		//    ����renderGame() ��Ⱦ��Ϸ����
		//    ����swingHook() ���Ӱڶ�
		//    ʹ��(int endTime = clock()) ������ʱ
		//    Time.second -= frameTime
		//    sleep(frameTime - (endTime - startTime))
		double frameTime = 1000 / 60;
		int startTime = clock();
		renderGame(currentTheme, hook, map, countdown);
		int endTime = clock();
		/*countdown.miliseconds -= frameTime / 8;*/
		if (frameTime - (endTime - startTime) > 0)
			Sleep(frameTime - (endTime - startTime));
		//���������룺
		//    �¼������¹���״̬��1.������ ����launchHook() ���乳��
		//    ESC������pauseView() ��ͣ��Ϸ

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
						//������Ϸ����Ч
						if (gameSoundFlag == 1) PlaySound("./assests/goldTheme/sound.wav", NULL, SND_FILENAME | SND_ASYNC);

						cout << "�¼��Ѱ���" << endl;
						hook.status = 1;
						launchHook(countdown, hook, map, gameStatus);
						break;
					case VK_ESCAPE:
						cout << "�˳����Ѱ���" << endl;
						stopFlag.store(true);
						pauseView(currentTheme, gameStatus, map);
						break;
					}
				}
			}
		}

		//�ж�(����״̬ == 2.����) ����pullHook() ��������*/

		if (hook.status == 2)
		{
			pullHook(countdown, hook, map, gameStatus);
			hook.getitem = {};//���ӻ���֮������Ϊ�չ�
		}

		//�ж�(����ʱ == 0)��
		//    �ж�(isWin() == true)������winGame()
		//    �ж�(isWin() == false)������loseGame()

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
//----------------------------------------------------��Ϸ����--------------------------------------------------------------------
//-----------------------------------------------------�������--------------------------------------------------------------------
int main() {
	//�����ⲿ�����
	AddFontResourceEx("font.ttf", FR_PRIVATE, NULL);
	initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
	setbkcolor(WHITE);
	thread countdownrThread(countDown, ref(countdown));//�����ʱ�߳�
	//Ĭ������100%
	waveOutSetVolume(0, 0xFFFFFFFF);
	cleardevice();
	loginView();
	//ɾ���ⲿ�����
	RemoveFontResourceEx("font.ttf", FR_PRIVATE, NULL);
	system("pause");
}