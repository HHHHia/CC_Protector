#include <iostream>
#include <graphics.h>
#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>
using namespace std;

const int windowWidth = 1280;
const int windowHeight = 720;

bool isGameStarted = false;
bool running = true;

const int buttonWidth = 192;
const int buttonHeight = 75;

int score = 0;
int historyScore = 0;

int bulletNum = 2;

#pragma comment(lib,"MSIMG32.LIB")
#pragma comment(lib,"Winmm.lib")

enum class GameState {
	MENU,
	PLAYING,
	GAMEOVER
};

GameState gameState = GameState::MENU;

//inline 是为了减少调用函数的开销
inline void putImage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, bf);
}

class Picture {
public:
	Picture(LPCTSTR path, int num) {
		TCHAR path_file[256];
		for (size_t i = 0; i <= num; i++) {
			_stprintf_s(path_file, path, i);
			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
	}

	~Picture() {
		for (size_t i = 0; i < frame_list.size(); i++)
			delete frame_list[i];
	}

public:
	vector<IMAGE*>frame_list;
};

class Button {
public:
	Button(RECT rect, LPCTSTR pImg_idle, LPCTSTR pImg_hovered, LPCTSTR pImg_pushed) {
		region = rect;
		loadimage(&img_idle, pImg_idle);
		loadimage(&img_hovered, pImg_hovered);
		loadimage(&img_pushed, pImg_pushed);
	}

	~Button() = default;

	void ProcessEvent(const ExMessage& msg) {
		switch (msg.message) {
		case WM_MOUSEMOVE:
			if (status == Status::Idle && checkMouseHit(msg.x, msg.y))
				status = Status::Hovered;
			else if (status == Status::Hovered && !checkMouseHit(msg.x, msg.y))
				status = Status::Idle;
			break;
		case WM_LBUTTONDOWN:
			if (checkMouseHit(msg.x, msg.y))
				status = Status::Pushed;
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed)
				OnClick();
			break;
		default:
			break;
		}
	}

	void Draw() {
		switch (status) {
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered);
			break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed);
			break;
		}
	}

protected:
	virtual void OnClick() = 0;

private:
	bool checkMouseHit(int x, int y) {
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	}

private:
	enum class Status {
		Idle = 0,
		Hovered,
		Pushed
	};

private:
	RECT region;
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status = Status::Idle;
};

class StartGameButton : public Button {
public:
	StartGameButton(RECT rect, LPCTSTR pImg_idle, LPCTSTR pImg_hovered, LPCTSTR pImg_pushed)
		:Button(rect, pImg_idle, pImg_hovered, pImg_pushed) {
	}

	~StartGameButton() = default;

protected:
	void OnClick() {
		gameState = GameState::PLAYING;
		isGameStarted = true;
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);//play BGM
	}
};

class EndGameButton : public Button {
public:
	EndGameButton(RECT rect, LPCTSTR pImg_idle, LPCTSTR pImg_hovered, LPCTSTR pImg_pushed)
		:Button(rect, pImg_idle, pImg_hovered, pImg_pushed) {
	}
	~EndGameButton() = default;

protected:
	void OnClick() {
		running = false;
	}
};

class Animation {
public:
	Animation(Picture* picture, int interval) {//load 图片				
		interval_ms = interval;
		animationPicture = picture;
	}

	~Animation() = default;

	void play(int x, int y, int delta) {//动画循环时间
		timer += delta;
		if (timer >= interval_ms) {
			idx_frame = (idx_frame + 1) % animationPicture->frame_list.size();
			timer = 0;
		}
		putImage_alpha(x, y, animationPicture->frame_list[idx_frame]);
	}

private:
	int timer = 0;
	int idx_frame = 0;
	int interval_ms = 0;

private:
	Picture* animationPicture;
};

Picture* picturePlayerLeft;
Picture* picturePlayerRight;
Picture* picture_F_EnemyLeft;
Picture* picture_F_EnemyRight;
Picture* picture_E_EnemyLeft;
Picture* picture_E_EnemyRight;
Picture* picture_Buff;

class Player {
public:
	Player() {//load图片
		loadimage(&img_shadow, _T("img/shadow_player.png"));
		anim_Left = new Animation(picturePlayerLeft, 45);
		anim_Right = new Animation(picturePlayerRight, 45);

	}

	~Player() {
		delete anim_Left;
		delete anim_Right;
	}

	void processEvent(const ExMessage& msg) {//接收消息
		switch (msg.message) {
		case WM_KEYDOWN:
			switch (msg.vkcode) {
			case VK_UP:
			case 'W':
			case 'w':
				isMoveUp = true;
				break;
			case VK_DOWN:
			case 'S':
			case 's':
				isMoveDown = true;
				break;
			case VK_LEFT:
			case 'A':
			case 'a':
				isMoveLeft = true;
				break;
			case VK_RIGHT:
			case 'D':
			case 'd':
				isMoveRight = true;
				break;
			}
			break;
		case WM_KEYUP:
			switch (msg.vkcode) {
			case VK_UP:
			case 'W':
			case 'w':
				isMoveUp = false;
				break;
			case VK_DOWN:
			case 'S':
			case 's':
				isMoveDown = false;
				break;
			case VK_LEFT:
			case 'A':
			case 'a':
				isMoveLeft = false;
				break;
			case VK_RIGHT:
			case 'D':
			case 'd':
				isMoveRight = false;
				break;
			}
			break;
		default:
			break;
		}
	}


	void move() {//移动
		int dir_x = isMoveRight - isMoveLeft;
		int dir_y = isMoveDown - isMoveUp;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);

		if (len_dir != 0) {
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			Position.x += (int)(speed * normalized_x);
			Position.y += (int)(speed * normalized_y);
		}

		if (Position.x < 0)
			Position.x = 0;

		if (Position.y < 0)
			Position.y = 0;

		if (Position.x + playerWidth > windowWidth)
			Position.x = windowWidth - playerWidth;

		if (Position.y + playerHeight > windowHeight)
			Position.y = windowHeight - playerHeight;

	}

	void draw(int delta) {//生成主角，影子和换朝向

		int posShadow_X = Position.x + (playerWidth / 2 - shadowWidth / 2);
		int posShadow_Y = Position.y + playerHeight - 8;
		putImage_alpha(posShadow_X, posShadow_Y, &img_shadow);

		static bool facingLeft = false;
		int dir_x = isMoveRight - isMoveLeft;
		if (dir_x < 0)
			facingLeft = true;
		else if (dir_x > 0)
			facingLeft = false;

		if (facingLeft)
			anim_Left->play(Position.x, Position.y, delta);
		else
			anim_Right->play(Position.x, Position.y, delta);
	}

	const POINT& getPosition() const {
		return Position;
	}

	void reset() {
		Position = { 500,500 };
		isMoveUp = false;
		isMoveDown = false;
		isMoveLeft = false;
		isMoveRight = false;
	}

public:
	const int player_Width = 80;
	const int player_Height = 80;

private:
	//玩家数值
	int	speed = 5;
	const int playerWidth = 80;
	const int playerHeight = 80;
	const int shadowWidth = 32;

private:
	IMAGE img_shadow;
	Animation* anim_Left;
	Animation* anim_Right;
	POINT Position = { 500,500 };
	bool isMoveUp = false;
	bool isMoveDown = false;
	bool isMoveLeft = false;
	bool isMoveRight = false;

};

class Bullet {
public:
	Bullet() = default;
	~Bullet() = default;

	void Draw() const {//子弹颜色
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x, position.y, RADIUS);

	}

public:
	POINT position = { 0,0 };

private:
	const int RADIUS = 10;
};

class Enemy {
public:
	enum class EnemyType {
		F_Type,
		E_Type
	};
	Enemy() {
		//load 敌人
		loadimage(&img_shadow, _T("img/shadow_enemy.png"));
		anim_F_Left = new Animation(picture_F_EnemyLeft, 45);
		anim_F_Right = new Animation(picture_F_EnemyRight, 45);
		anim_E_Left = new Animation(picture_E_EnemyLeft, 45);
		anim_E_Right = new Animation(picture_E_EnemyRight, 45);

		int ran = rand() % 2;
		type = (ran == 0 ? EnemyType::F_Type : EnemyType::E_Type);

		if (type == EnemyType::F_Type) {
			speedE = 2;
			health = 3;
		}
		else { // E_Type
			speedE = 3;
			health = 1;
		}

		//敌人出生边界
		enum class SpawnEdge {
			up = 0,
			down,
			Left,
			Right
		};
		SpawnEdge edge = static_cast<SpawnEdge>(rand() % 4);
		switch (edge) {
		case SpawnEdge::up:
			Position.x = rand() % windowWidth;
			Position.y = -frameHeight;
			break;
		case SpawnEdge::down:
			Position.x = rand() % windowWidth;
			Position.y = windowHeight;
			break;
		case SpawnEdge::Left:
			Position.x = -frameHeight;
			Position.y = rand() % windowHeight;
			break;
		case SpawnEdge::Right:
			Position.x = windowWidth;
			Position.y = rand() % windowHeight;
			break;
		default:
			break;
		}
	}

	~Enemy() {
		delete anim_F_Left;
		delete anim_F_Right;
		delete anim_E_Left;
		delete anim_E_Right;
	}

	bool checkBulletResult(const Bullet& bullet) {
		//判断子弹是否在敌人内
		bool isOverLap_X = bullet.position.x >= Position.x &&
			bullet.position.x <= Position.x + frameWidth;
		bool isOverLap_Y = bullet.position.y >= Position.y &&
			bullet.position.y <= Position.y + frameHeight;
		return  isOverLap_X && isOverLap_Y;
	}

	bool checkPlayerEnemy(const Player& player) {//检查敌人和主角有没有碰撞
		POINT playerPos = player.getPosition();
		int playerLeft = playerPos.x;
		int playerRight = playerPos.x + player.player_Width;
		int playerTop = playerPos.y;
		int playerBottom = playerPos.y + player.player_Height;

		// 敌人的矩形区域
		int enemyLeft = Position.x + 50;
		int enemyRight = Position.x + frameWidth - 50;
		int enemyTop = Position.y + 50;
		int enemyBottom = Position.y + frameHeight - 50;

		// 如果两个矩形没有重叠，则返回 false
		if (playerRight < enemyLeft || playerLeft > enemyRight ||
			playerBottom < enemyTop || playerTop > enemyBottom) {
			return false;
		}
		// 否则认为碰撞发生，返回 true
		return true;
	}

	void move(const Player& player) {//敌人跟随主角移动
		const POINT& playerPosition = player.getPosition();
		int dir_x = playerPosition.x - Position.x;
		int dir_y = playerPosition.y - Position.y;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0) {
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			Position.x += static_cast<int>(speedE * normalized_x);
			Position.y += static_cast<int>(speedE * normalized_y);
		}
		if (dir_x < 0)
			facingLeft = true;
		else if (dir_x > 0)
			facingLeft = false;
	}

	void Draw(int delta) {//画敌人
		int posShadow_X = Position.x + (playerWidth / 2 - shadowWidth / 2);
		int posShadow_Y = Position.y + playerHeight - 20;
		putImage_alpha(posShadow_X, posShadow_Y, &img_shadow);

		if (type == EnemyType::F_Type) {
			if (facingLeft)
				anim_F_Left->play(Position.x, Position.y, delta);
			else
				anim_F_Right->play(Position.x, Position.y, delta);
		}
		else {  // EnemyType::E_Type
			if (facingLeft)
				anim_E_Left->play(Position.x, Position.y, delta);
			else
				anim_E_Right->play(Position.x, Position.y, delta);
		}
	}

	void hurt(int damageTaken = 1) {
		health -= damageTaken;
		if (health <= 0)
			alive = false;
	}

	bool tryDamage(const Bullet* bullet, int damageTaken = 1, DWORD cooldown = 200) {
		DWORD now = GetTickCount();
		auto it = damageCooldown.find(bullet);
		if (it == damageCooldown.end() || (now - it->second >= cooldown)) {
			// 超过冷却时间或未记录过，则执行伤害
			hurt(damageTaken);
			damageCooldown[bullet] = now;  // 更新该颗子弹的伤害时刻
			return true;
		}
		return false;
	}

	bool checkAlive() {
		return alive;
	}

private:
	EnemyType type;
	int health;
	unordered_map<const Bullet*, DWORD> damageCooldown;
private:
	//敌人数值 E
	int	speedE = 2;
	const int frameWidth = 80;
	const int frameHeight = 80;
	const int shadowWidth = 40;

private:
	const int playerWidth = 80;
	const int playerHeight = 80;

private:
	IMAGE img_shadow;
	Animation* anim_F_Left;
	Animation* anim_F_Right;
	Animation* anim_E_Left;
	Animation* anim_E_Right;
	POINT Position = { 500,500 };
	bool facingLeft = false;
	bool alive = true;
};

class Buff {
public:
	Buff() {
		//load Buff
		loadimage(&img_shadow, _T("img/shadow_enemy.png"));
		img_Buff = new Animation(picture_Buff, 45);

		//Buff出生
		Position.x = rand() % windowWidth;
		Position.y = rand() % frameHeight;

	}

	~Buff() {
		delete img_Buff;
	};

	bool checkPlayerBuff(const Player& player) {//检查敌人和主角有没有碰撞
		POINT playerPos = player.getPosition();
		int playerLeft = playerPos.x;
		int playerRight = playerPos.x + player.player_Width;
		int playerTop = playerPos.y;
		int playerBottom = playerPos.y + player.player_Height;

		// 敌人的矩形区域
		int buffLeft = Position.x + 50;
		int buffRight = Position.x + frameWidth - 50;
		int buffTop = Position.y + 50;
		int buffBottom = Position.y + frameHeight - 50;

		// 如果两个矩形没有重叠，则返回 false
		if (playerRight < buffLeft || playerLeft > buffRight ||
			playerBottom < buffTop || playerTop > buffBottom) {
			return false;
		}
		// 否则认为碰撞发生，返回 true
		return true;
	}

	void Draw(int delta) {//画Buff
		int posShadow_X = Position.x + (playerWidth / 2 - shadowWidth / 2);
		int posShadow_Y = Position.y + playerHeight - 20;
		putImage_alpha(posShadow_X, posShadow_Y, &img_shadow);
		img_Buff->play(Position.x, Position.y, delta);
	}

private:
	//数值
	const int frameWidth = 50;
	const int frameHeight = 67;
	const int shadowWidth = 40;

private:
	const int playerWidth = 80;
	const int playerHeight = 80;

private:
	IMAGE img_shadow;
	Animation* img_Buff;
	POINT Position = { 500,500 };
};

void tryGenerateBuff(vector<Buff*>& buff_List) {
	const int INTERVAL = 300;  // 每隔 300 帧左右产生一个 buff
	static int counter = 0;
	counter++;
	if (counter % INTERVAL == 0) {
		buff_List.push_back(new Buff());
	}
}

void tryGenerateEnemy(vector<Enemy*>& enemy_List) {//生成敌人
	const int INTERVAL = 100;
	static int counter = 0;
	if ((++counter) % INTERVAL == 0) {
		enemy_List.push_back(new Enemy());
	}
}

void updateBullet(vector<Bullet>& bullet_list, const Player& player) {//生成子弹
	const double radiusSpeed = 0.0055;//子弹波动速度
	const double circleSpeed = 0.0045;//子弹围绕主角速度
	double bulletInterval = 2 * 3.14159 / bullet_list.size();//子弹间隔
	POINT playerPosition = player.getPosition();
	double radius = 100 + 25 * sin(GetTickCount() * radiusSpeed);//角度随时间变化
	for (size_t i = 0; i < bullet_list.size(); i++) {//改变子弹的位置
		double radian = GetTickCount() * circleSpeed + bulletInterval * i;
		bullet_list[i].position.x = playerPosition.x + player.player_Width / 2 + (int)(radius * sin(radian));
		bullet_list[i].position.y = playerPosition.y + player.player_Height / 2 + (int)(radius * cos(radian));
	}
}

char getGrade(int score) {
	if (score <= 20)
		return 'F';
	else if (score <= 40)
		return 'E';
	else if (score <= 60)
		return 'D';
	else if (score <= 80)
		return 'C';
	else if (score <= 100)
		return 'B';
	else if (score <= 100)
		return 'A';
	else
		return 'S';
}

void drawPlayerScore(int score) {//展示分数
	static TCHAR text[64];
	_stprintf_s(text, _T("Your score is : %d"), score);
	static TCHAR text2[64];
	_stprintf_s(text2, _T("Your grade is : %c"), getGrade(score));
	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	settextstyle(50, 0, _T("Arial"));
	outtextxy(10, 10, text);
	outtextxy(10, 60, text2);
}

void resetGame(Player& player,
	vector<Enemy*>& enemy_List,
	vector<Bullet>& bullet_List) {
	score = 0;

	// 删除所有敌人
	for (Enemy* enemy : enemy_List) {
		delete enemy;
	}
	enemy_List.clear();

	player.reset();

	// 重新初始化子弹列表
	bullet_List.clear();
	bullet_List.push_back(Bullet());
	bullet_List.push_back(Bullet());

	mciSendString(_T("stop bgm"), NULL, 0, NULL);//停止音乐
}

void showHistoryScore() {
	static TCHAR text[64];
	_stprintf_s(text, _T("你的最高成绩是： %c"), getGrade(historyScore));
	setbkmode(TRANSPARENT);//透明背景
	settextcolor(RGB(200, 50, 15));
	settextstyle(30, 0, _T("Arial"));
	outtextxy(950, 650, text);
}

bool stopGame() {
	if (score >= 150)
		return true;
	else
		return false;
}

int main() {

	initgraph(1280, 720);//显示框大小

	picturePlayerLeft = new Picture(_T("img/player_left_%d.png"), 5);
	picturePlayerRight = new Picture(_T("img/player_RIght_%d.png"), 5);
	picture_F_EnemyLeft = new Picture(_T("img/enemyF_left_%d.png"), 0);
	picture_F_EnemyRight = new Picture(_T("img/enemyF_right_%d.png"), 0);
	picture_E_EnemyLeft = new Picture(_T("img/enemyE_left_%d.png"), 0);
	picture_E_EnemyRight = new Picture(_T("img/enemyE_right_%d.png"), 0);
	picture_Buff = new Picture(_T("img/Buff.png"), 0);

	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);//load BGM
	mciSendString(_T("open mus/hit.wav alias hit"), NULL, 0, NULL);//load hit effect

	//定义类
	Player player;
	ExMessage msg;
	IMAGE img_menu;
	IMAGE img_Background;
	vector<Enemy*>enemy_List;//敌人列表
	vector<Bullet>bullet_List(bulletNum);//子弹列表
	vector<Buff*>buff_list;

	RECT regionB_StartGame, regionB_EndGame;

	regionB_StartGame.left = (windowWidth - buttonWidth) / 2;
	regionB_StartGame.right = regionB_StartGame.left + buttonWidth;
	regionB_StartGame.top = 430;
	regionB_StartGame.bottom = regionB_StartGame.top + buttonHeight;

	regionB_EndGame.left = (windowWidth - buttonWidth) / 2;
	regionB_EndGame.right = regionB_EndGame.left + buttonWidth;
	regionB_EndGame.top = 550;
	regionB_EndGame.bottom = regionB_EndGame.top + buttonHeight;

	StartGameButton BSG(regionB_StartGame,
		_T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"),
		_T("img/ui_start_pushed.png"));
	EndGameButton BEG(regionB_EndGame,
		_T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"),
		_T("img/ui_quit_pushed.png"));

	//load 背景
	loadimage(&img_Background, _T("img/background.png"));
	loadimage(&img_menu, _T("img/menu.png"));

	BeginBatchDraw();

	while (running) {

		DWORD start_time = GetTickCount();

		while (peekmessage(&msg)) {//接受信息
			if (gameState == GameState::PLAYING)
				player.processEvent(msg);
			else {
				BSG.ProcessEvent(msg);
				BEG.ProcessEvent(msg);
			}
		}

		if (gameState == GameState::PLAYING) {
			player.move();

			updateBullet(bullet_List, player);

			//生成敌人
			tryGenerateEnemy(enemy_List);
			tryGenerateBuff(buff_list);

			for (Enemy* enemy : enemy_List)
				enemy->move(player);

			//战败
			for (Enemy* enemy : enemy_List) {
				if (enemy->checkPlayerEnemy(player)) {
					static TCHAR text[128];
					_stprintf_s(text, _T("你的成绩是 %c ！！!"), getGrade(score));
					MessageBox(GetHWnd(), text, _T("Game over!!"), MB_OK);
					gameState = GameState::GAMEOVER;
					resetGame(player, enemy_List, bullet_List);
					for (Buff* buff : buff_list)
						delete buff;
					buff_list.clear();
					isGameStarted = false;
					bulletNum = 2;
					break;
				}
			}

			DWORD currentTime = GetTickCount();
			for (Enemy* enemy : enemy_List) {//用循环去检查子弹是否在每一个敌人内
				for (const Bullet& bullet : bullet_List) {
					if (enemy->checkBulletResult(bullet)) {
						if (enemy->tryDamage(&bullet, 1, 200)) {  // 200毫秒冷却时间
							mciSendString(_T("play hit from 0"), NULL, 0, NULL);
							score++;
							if (historyScore < score)
								historyScore = score;
						}
					}
				}
			}

			if (stopGame()) {
				static TCHAR text3[128];
				_stprintf_s(text3, _T("你的成绩是 %c ！！!"), getGrade(score));
				MessageBox(GetHWnd(), text3, _T("You Win!!"), MB_OK);
				gameState = GameState::MENU;
				resetGame(player, enemy_List, bullet_List);
				for (Buff* buff : buff_list)
					delete buff;
				buff_list.clear();
				isGameStarted = false;
				bulletNum = 2;
			}

			// 检测 buff 与玩家的碰撞，若碰撞则增加子弹数并移除该 buff
			for (auto it = buff_list.begin(); it != buff_list.end(); ) {
				if ((*it)->checkPlayerBuff(player)) {
					bulletNum++;                 // 增加全局子弹数
					bullet_List.push_back(Bullet());  // 在子弹列表中添加一颗新子弹
					// 这里可以播放 buff 播放音效（如有需要）
					delete* it;
					it = buff_list.erase(it);
				}
				else {
					++it;
				}
			}

			for (size_t i = 0; i < enemy_List.size(); i++) {//删除已经死去的敌人
				Enemy* enemy = enemy_List[i];
				if (!enemy->checkAlive()) {
					swap(enemy_List[i], enemy_List.back());//将死亡敌人与类别最后元素交换
					enemy_List.pop_back();//去除最后的元素
					delete enemy;//删除掉
				}
			}
			if (gameState == GameState::GAMEOVER) {
				resetGame(player, enemy_List, bullet_List);
				gameState = GameState::MENU;
				isGameStarted = false; // 保持与现有逻辑兼容
			}

		}
		else if (gameState == GameState::GAMEOVER) {
			// 调用重置函数，将游戏状态重置为初始状态，再切换回 MENU
			resetGame(player, enemy_List, bullet_List);
			for (Buff* buff : buff_list)
				delete buff;
			buff_list.clear();
			gameState = GameState::MENU;
		}

		cleardevice();
		if (isGameStarted && gameState == GameState::PLAYING) {
			putimage(0, 0, &img_Background);//背景

			player.draw(1000 / 144);//load 玩家
			for (Enemy* enemy : enemy_List)//load 敌人
				enemy->Draw(1000 / 144);
			for (const Bullet& bullet : bullet_List)//画子弹
				bullet.Draw();
			for (Buff* buff : buff_list)
				buff->Draw(1000 / 144);
			drawPlayerScore(score);
		}
		else {
			putimage(0, 0, &img_menu);
			BSG.Draw();
			BEG.Draw();
			showHistoryScore();
		}

		FlushBatchDraw();

		DWORD end_time = GetTickCount();
		DWORD delta_time = end_time - start_time;
		if (delta_time < 1000 / 144) {
			Sleep(1000 / 144 - delta_time);
		}//让while 别运行那么快，少占用内存

	}

	for (Enemy* enemy : enemy_List)
		delete enemy;
	enemy_List.clear();//释放内存

	//resetGame(player, enemy_List, bullet_List);

	delete picturePlayerLeft;
	delete picturePlayerRight;
	delete picture_F_EnemyLeft;
	delete picture_F_EnemyRight;
	delete picture_E_EnemyLeft;
	delete picture_E_EnemyRight;
	delete picture_Buff;


	EndBatchDraw();

	return 0;
}
