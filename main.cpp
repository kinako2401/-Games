#include "DxLib.h"
#include <cstdlib>
#include <ctime>
#include <cwchar>

// 四角形を管理する構造体
struct Square {
    int x, y;          // 位置
    int size;          // サイズ
    int color;         // 色
};

// 命中エフェクトを管理する構造体
struct HitEffect {
    int x, y;          // エフェクトの位置
    int size;          // エフェクトのサイズ（半径）
    int life;          // エフェクトの残り時間（ミリ秒）
    int maxSize;       // エフェクトが最大になるサイズ
    int color;         // エフェクトの色
};

// 命中エフェクトを生成
static void GenerateHitEffect(int x, int y, HitEffect& effect) {
    effect.x = x;
    effect.y = y;
    effect.size = 10; // 初期のサイズ（半径）
    effect.maxSize = 100; // 最大サイズ（半径）
    effect.life = 500; // エフェクトのライフ（500msで消える）
    effect.color = GetColor(255, 0, 0); // 赤色
}

// エフェクトの描画
static void DrawHitEffect(const HitEffect& effect) {
    if (effect.life > 0) {
        int thick = 10; // 枠線の太さ

        // 枠線の太さを出すために、同じ位置で円を何回か描画
        for (int i = 0; i < thick; i++) {
            // 枠線を少しずつ広げて描画する
            DrawCircle(effect.x, effect.y, effect.size + i, effect.color, FALSE);
        }
    }
}

// 時間経過でエフェクトを更新する
static void UpdateHitEffect(HitEffect& effect) {
    if (effect.life > 0) {
        effect.size += 2;  // サイズを少しずつ大きくする
        if (effect.size > effect.maxSize) {
            effect.size = effect.maxSize;  // 最大サイズを超えないようにする
        }
        effect.life -= 10;  // 10msごとにライフを減らす
    }
}

// 四角形を生成
static void GenerateSquare(Square& square, int size)
{
    square.size = size;
    square.x = rand() % (1280 - square.size); // ランダムなX座標
    square.y = rand() % (760 - square.size); // ランダムなY座標
    square.color = GetColor(255, 255, 255); // 白色

    // 画面外に出ないように座標を調整
    if (square.x > 1280 - square.size) {
        square.x = 1280 - square.size;
    }
    if (square.y > 720 - square.size) {
        square.y = 720 - square.size;
    }
}
// ランクを管理する構造体
struct Rank {
    const wchar_t* name;     // ランク名
    double minAccuracy;      // 最低正確性
    int minScore;            // 最低スコア
    const wchar_t* title;    // 称号
};

Rank ranks[] = {
    {L"SSS", 100.0, 45, L"神の領域"},
    {L"SS",  95.0,  40, L"超絶エイマー"},
    {L"S",   90.0,  35, L"天才スナイパー"},
    {L"A",   85.0,  30, L"安定のエイマー"},
    {L"B+",  80.0,  25, L"そこそこ射撃手"},
    {L"B",   70.0,  20, L"普通の射手"},
    {L"B-",  60.0,  15, L"惜しいエイム"},
    {L"C+",  50.0,  10, L"初心者エイム"},
    {L"C",   40.0,   5, L"未熟な射撃手"},
    {L"C-",  30.0,   3, L"経験不足"},
    {L"D",   20.0,   1, L"訓練が必要"},
    {L"E",    0.0,   0, L"......"},
};
const int rankCount = sizeof(ranks) / sizeof(ranks[0]);

static const Rank& GetPlayerRank(double per, int score) {
    for (int i = 0; i < rankCount; i++) {
        if (per >= ranks[i].minAccuracy && score >= ranks[i].minScore) {
            return ranks[i];
        }
    }
    return ranks[rankCount - 1]; // 最低ランク（E）
}


static bool CheckClick(int mx, int my, const Square& square) {
    // マウス座標が四角形の内部にあるかを判定
    return mx >= square.x && mx <= square.x + square.size &&
        my >= square.y && my <= square.y + square.size;
}


int effectCount = 0;
HitEffect hitEffects[10];  // 最大10個のエフェクトを保持
int countdown = 3;  // カウントダウンの初期値 (3秒からスタート)
int countdownTimer = 0;  // カウントダウンのタイマー（ミリ秒）
bool countdownActive = true;  // カウントダウン中かどうか

int WINAPI WinMain(						//Windows画面の設定
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    // ----- 定数の宣言 ----- //
    int SCREEN_WIDTH;
    int SCREEN_HEIGHT;

    // 現在の画面の解像度を取得
    SCREEN_WIDTH = GetSystemMetrics(SM_CXSCREEN);  // 画面の幅を取得
    SCREEN_HEIGHT = GetSystemMetrics(SM_CYSCREEN); // 画面の高さを取得

    // ----- 変数の宣言 ----- //
    
    int font;               // 文字用フォント
    int titlefont;          // タイトル用フォント
    int resultpoint;        // リザルト表示用座標
    int score;              // スコア計算
    int click;              // クリック計算
    int squareSize;         // 的の大きさ
    int startTime;          // ゲーム開始時刻（ミリ秒）
    double per;             // エイムの正確性
    int hit;                // 命中音
    int countdownSound;

    // ----- 変数の初期化 ----- //

    font = 0;                   // 文字用フォント
    titlefont = 0;              // タイトル用フォント
    resultpoint = 400;          // リザルト表示用座標
    score = 0;                  // スコア計算
    click = 0;                  // クリック計算
    squareSize = 200;           //的の大きさ
    startTime = GetNowCount();  // ゲーム開始時刻（ミリ秒）
    per = 0.0;                    // エイムの正確性

    // 画面モードのセット (PCの画面サイズに合わせる)
    SetGraphMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32); //(x座標,y座標,色数)

    //ウインドウモードへ変更
    ChangeWindowMode(TRUE);

    //タイトルの変更
    SetMainWindowText(L"AIM_GAME");

    if (DxLib_Init() == -1)	// ＤＸライブラリ初期化処理
    {
        return -1;				// エラーが起きたら直ちに終了
    }

    // フォントサイズの設定
    font = CreateFontToHandle(L"メイリオ", 50, -1, DX_FONTTYPE_ANTIALIASING); // フォントサイズを50に設定
    titlefont = CreateFontToHandle(L"メイリオ", 100, -1, DX_FONTTYPE_ANTIALIASING); // フォントサイズを50に設定

    hit = LoadSoundMem(L"命中.mp3");
    if (hit == -1) {
        MessageBox(NULL, L"効果音ファイルの読み込みに失敗しました", L"エラー", MB_OK);
        DxLib_End();
        return -1;
    }

    countdownSound = LoadSoundMem(L"カウントダウン.mp3");
    if (countdownSound == -1) {
        MessageBox(NULL, L"カウントダウン音声の読み込みに失敗しました", L"エラー", MB_OK);
        DxLib_End();
        return -1;
    }

    // ランダムシードの初期化
    srand(static_cast<unsigned int>(time(0)));

    // 四角形の初期化
    Square square;
    GenerateSquare(square, squareSize);

    bool isMousePressed = false;  // マウスの左ボタンが押されているかどうか

    while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        // ----- 変数の初期化 ----- //
        score = 0;                  // スコア計算
        click = 0;                  // クリック計算
        squareSize = 200;           //的の大きさ
        startTime = GetNowCount();  // ゲーム開始時刻（ミリ秒）
        per = 0.0;                    // エイムの正確性

        // ----- タイトル画面----- //
        while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
        {
            // 画面を初期化する
            ClearDrawScreen();

            // タイトル表示
            DrawFormatStringToHandle(450, SCREEN_HEIGHT / 2 - 300, GetColor(255, 255, 255), titlefont, L"AIM_MASTER");

            // メッセージを表示（ゲームスタート前）
            DrawFormatStringToHandle(490, SCREEN_HEIGHT / 2 + 100, GetColor(255, 255, 255), font, L"Press to [SPACE] key");

            // ----- ランク一覧の表示 ----- //
            DrawString(50, 70, L"RANK LIST:", GetColor(200, 200, 0));
            for (int i = 0; i < rankCount; i++) {
                int y = 100 + i * 30;  // 縦に30ピクセルずつずらして描画
                DrawFormatString(70, y, GetColor(255, 255, 255), L"- % s", ranks[i].name);
            }

            ScreenFlip();

            WaitTimer(100);

            // SPACEキーが押されたらゲームスタート
            if (CheckHitKey(KEY_INPUT_SPACE)) {
                break;
            }
        }

        wchar_t countStr[16];  // 数値の文字列変換用バッファ

        while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
        {
            for (int countdown = 3; countdown >= 0; countdown--) {
                PlaySoundMem(countdownSound, DX_PLAYTYPE_BACK);

                const wchar_t* text = nullptr;

                if (countdown > 0) {
                    _itow_s(countdown, countStr, 10); // int → wchar_t*
                    text = countStr;
                }
                else {
                    text = L"GO!";
                }

                // アニメーション時間（ms）
                int duration = 800;
                int startTime = GetNowCount();

                while (GetNowCount() - startTime < duration) {
                    ClearDrawScreen();

                    int now = GetNowCount() - startTime;
                    float progress = now / static_cast<float>(duration);

                    float scale = 1.0f + progress * 1.0f;
                    int alpha = static_cast<int>((1.0f - progress) * 255);

                    int drawSize = static_cast<int>(100 * scale); // 拡大スケーリング
                    int tempFont = CreateFontToHandle(L"メイリオ", drawSize, -1, DX_FONTTYPE_ANTIALIASING);

                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

                    int textWidth = GetDrawStringWidthToHandle(text, wcslen(text), tempFont);
                    DrawStringToHandle(SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - drawSize / 2, text, GetColor(255, 255, 255), tempFont);

                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    DeleteFontToHandle(tempFont);

                    ScreenFlip();
                    WaitTimer(16); // 約60FPS
                }
            }
            break;
        }
        

        startTime = GetNowCount();


        // ----- ゲーム開始 ----- //
        while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
        {
            // ---------- 更新処理 ---------- //

            int mx, my;
            GetMousePoint(&mx, &my);

            // マウスの左ボタンの状態を取得
            int mouseInput = GetMouseInput();

            // ----- クリック判定 ----- //
            if ((mouseInput & MOUSE_INPUT_LEFT) && !isMousePressed) {  // クリックされた瞬間だけ
                isMousePressed = true;  // クリックが押されたとフラグを立てる
                click++;
                // マウスの左ボタンがクリックされた場合
                if (CheckClick(mx, my, square)) {
                    // 四角形をクリックした場合、スコアを加算して新たな四角形を生成
                    score++;  // スコアを加算
                    PlaySoundMem(hit, DX_PLAYTYPE_BACK);
                    GenerateSquare(square, squareSize);  // 新しい四角形を生成
                    // 命中エフェクトを追加
                    if (effectCount < 10) {
                        GenerateHitEffect(mx, my, hitEffects[effectCount]);
                        effectCount++;
                    }

                }
            }

            // マウスが離れた場合にクリック状態をリセット
            if (!(mouseInput & MOUSE_INPUT_LEFT)) {
                isMousePressed = false;  // クリック状態をリセット
            }

            // エフェクトを更新
            for (int i = 0; i < effectCount; i++) {
                UpdateHitEffect(hitEffects[i]);  // エフェクトの更新
                if (hitEffects[i].life <= 0) {
                    // エフェクトのライフが終わったら削除
                    for (int j = i; j < effectCount - 1; j++) {
                        hitEffects[j] = hitEffects[j + 1];
                    }
                    effectCount--;
                    i--;
                }
            }


            if (square.size <= 0 || square.x < 0 || square.y < 0 || square.x + square.size > SCREEN_WIDTH || square.y + square.size > SCREEN_HEIGHT) {
                DrawString(10, 120, L"描画範囲エラー", GetColor(255, 0, 0));
            }

            int elapsedTime = (GetNowCount() - startTime) / 1000;  // 経過秒数
            if (elapsedTime >= 20) {
                break;  // ゲーム終了
            }

            // ---------- 描画処理 ---------- //

            // 画面のクリア (毎フレーム最初に画面をクリアする)
            ClearDrawScreen();

            // ----- 四角形の描画 ----- //
            DrawBox(square.x + 1, square.y + 1, square.x + square.size - 1, square.y + square.size - 1, square.color, TRUE); // 内部 白
            DrawBox(square.x + (square.size / 16), square.y + (square.size / 16), (square.x + square.size) - (square.size / 16), (square.y + square.size) - (square.size / 16), GetColor(255, 0, 0), TRUE); // 真ん中 赤
            DrawBox(square.x + (square.size / 8), square.y + (square.size / 8), (square.x + square.size) - (square.size / 8), (square.y + square.size) - (square.size / 8), square.color, TRUE); // 真ん中 白
            DrawBox(square.x + (square.size / 4), square.y + (square.size / 4), (square.x + square.size) - (square.size / 4), (square.y + square.size) - (square.size / 4), GetColor(255, 0, 0), TRUE); // 真ん中 赤

            // ----- 命中エフェクトの描画 ----- //
            for (int i = 0; i < effectCount; i++) {
                DrawHitEffect(hitEffects[i]);  // 各エフェクトを描画
            }

            // ----- スコアの表示 ----- //
            DrawString(10, 10, L"Score: ", GetColor(255, 255, 255));
            DrawFormatStringToHandle(100, 10, GetColor(100, 100, 100), font, L"%d", score);
            // ----- クリック回数の表示 ----- //
            DrawString(10, 60, L"Click: ", GetColor(255, 255, 255));
            DrawFormatStringToHandle(100, 60, GetColor(100, 100, 100), font, L"%d", click);

            // ----- 残り時間の表示 ----- //
            DrawString(1400, 10, L"Time: ", GetColor(255, 255, 255));
            DrawFormatStringToHandle(1450, 10, GetColor(100, 100, 100), font, L"%d", (20 - elapsedTime));

            // ----- 画面の更新 ----- //
            ScreenFlip();
        }
        while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
        {
            // -------- リザルト画面 -------- //

            // 正確性の計算処理
            if (click != 0) { // クリック回数が0でないとき
                per = (static_cast<double>(score) / click) * 100.0;
            }
            else { // クリック回数が0の時
                per = 0.0;
            }

            // ランク取得
            Rank resultRank = GetPlayerRank(per, score);

            ClearDrawScreen();
            DrawFormatStringToHandle(SCREEN_WIDTH / 2 - resultpoint + 50, 100, GetColor(255, 255, 255), titlefont, L"--- RESULT ---");
            DrawFormatStringToHandle(resultpoint, SCREEN_HEIGHT / 2 - 150, GetColor(255, 255, 255), font, L"Score: %d", score);
            DrawFormatStringToHandle(resultpoint, SCREEN_HEIGHT / 2 - 50, GetColor(255, 255, 255), font, L"Click: %d", click);
            DrawFormatStringToHandle(resultpoint, SCREEN_HEIGHT / 2 + 50, GetColor(255, 255, 255), font, L"Per: %.1f%%", per);
            DrawFormatStringToHandle(400, SCREEN_HEIGHT / 2 + 150, GetColor(255, 255, 0), font, L"Rank: %s", resultRank.name);
            DrawFormatStringToHandle(400, SCREEN_HEIGHT / 2 + 250, GetColor(100, 255, 100), font, L"Title: %s", resultRank.title);
            DrawString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 350, L"[SPACE]キーを押すと終了", GetColor(200, 200, 200));
            ScreenFlip();

            // SPACEキーが押されたらタイトルに戻る
            if (CheckHitKey(KEY_INPUT_SPACE)) {
                break;
            }
        }
    }
    DeleteSoundMem(countdownSound);
    DeleteSoundMem(hit); // 効果音の解放

    DxLib_End(); // DxLibの終了

    return 0;
}
