#define _CRT_SECURE_NO_WARNINGS
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <ctime>   
#include <cstdlib> 
#include <cstdio> 

// --- OS SPECIFIC STUFF ---
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <direct.h>
#define CREATE_DIR(x) _mkdir(x)
void flushKeys() {
    while (_kbhit()) { _getch(); }
}
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#define CREATE_DIR(x) mkdir(x, 0777)
char _getch() {
    system("stty raw -echo");
    char ch = getchar();
    system("stty sane");
    return ch;
}
void flushKeys() {
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    while (getchar() != EOF);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
}
#endif

using namespace std;

// --- AUDIO SYSTEM ---
ma_engine g_engine;
ma_sound g_bgm_sound;

void initAudio() {
    ma_result result = ma_engine_init(NULL, &g_engine);
    if (result != MA_SUCCESS) {
        cout << "[AUDIO ERROR] Failed to initialize audio engine!\n";
        return;
    }
    cout << "[AUDIO] Engine initialized successfully!\n";
}

void cleanupAudio() {
    ma_engine_uninit(&g_engine);
}

void playBGM() {
    // Make sure filename matches exactly (Linux is case sensitive)
    ma_result result = ma_sound_init_from_file(&g_engine, "music-return-to-the-8-bit-past.wav", 0, NULL, NULL, &g_bgm_sound);
    
    if (result != MA_SUCCESS) {
        cout << "\n[AUDIO ERROR] BGM failed to load!\n";
        cout << "Check if 'music-return-to-the-8-bit-past.wav' is in the same folder as the executable.\n";
        return;
    }
    
    ma_sound_set_looping(&g_bgm_sound, MA_TRUE);
    ma_sound_start(&g_bgm_sound);
    ma_sound_set_volume(&g_bgm_sound, 0.20f); // Keep it a bit quiet so it doesn't overpower
    cout << "[AUDIO] BGM started successfully!\n";
}

void stopBGM() {
    ma_sound_stop(&g_bgm_sound);
    ma_sound_uninit(&g_bgm_sound);
}

// Generates a quick beep using miniaudio waveforms
void playSFX(string type) {
    double frequency = 440.0;
    double volume = 0.1;
    int duration_ms = 50;

    // Tweak the sound profiles here
    if (type == "key")    { frequency = 1200.0; duration_ms = 30;  volume = 0.05; }
    else if (type == "enter") { frequency = 800.0;  duration_ms = 60;  volume = 0.1; }
    else if (type == "scene") { frequency = 500.0;  duration_ms = 80;  volume = 0.1; }
    else if (type == "error") { frequency = 200.0;  duration_ms = 200; volume = 0.15; }
    else if (type == "tick")  { frequency = 1500.0; duration_ms = 15;  volume = 0.05; }
    else if (type == "crit")  { frequency = 1000.0; duration_ms = 100; volume = 0.15; }

    ma_uint32 sampleRate = ma_engine_get_sample_rate(&g_engine);

    ma_waveform_config waveformConfig = ma_waveform_config_init(
        ma_format_f32, 1, sampleRate, ma_waveform_type_sine, frequency, volume
    );

    ma_waveform waveform;
    if (ma_waveform_init(&waveformConfig, &waveform) != MA_SUCCESS) return;

    ma_sound sound;
    if (ma_sound_init_from_data_source(&g_engine, &waveform, 0, NULL, &sound) != MA_SUCCESS) {
        ma_waveform_uninit(&waveform);
        return;
    }

    ma_sound_start(&sound);
    this_thread::sleep_for(chrono::milliseconds(duration_ms));

    // Clean up
    ma_sound_stop(&sound);
    ma_sound_uninit(&sound);
    ma_waveform_uninit(&waveform);
}

// Custom input function so we can play a sound on every keystroke
string getInput(string prompt) {
    cout << prompt;
    string input = "";
    while (true) {
        char ch = _getch();
        
        if (ch == '\r' || ch == '\n') { // Enter
            cout << "\n";
            playSFX("enter");
            break;
        } 
        else if (ch == 127 || ch == 8) { // Backspace
            if (!input.empty()) {
                input.pop_back();
                cout << "\b \b" << flush; 
                playSFX("key");
            }
        } 
        else if (ch >= 32 && ch <= 126) { // Printable chars
            input += ch;
            cout << ch << flush;
            playSFX("tick");
        }
    }
    return input;
}

// --- ACCOUNT SYSTEM ---
struct PlayerAccount {
    string username = "";
    string password = "";
    int coins = 0;
    int maxHp = 0;
    int baseDamage = 0;
    int overallLevel = 0;
    int muscleLevel = 0;
    int vitalityLevel = 0;
};

// --- UI & UTILITIES ---
void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void gotoxy(int x, int y) {
    #ifdef _WIN32
        COORD coord;
        coord.X = x;
        coord.Y = y;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    #else
        cout << "\033[" << y + 1 << ";" << x + 1 << "H" << flush;
    #endif
}

void setColor(int color) {
    #ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    #else
        if (color == 10) cout << "\033[1;32m";
        else if (color == 14) cout << "\033[1;33m";
        else if (color == 12) cout << "\033[1;31m";
        else if (color == 11) cout << "\033[1;36m";
        else if (color == 13) cout << "\033[1;35m";
        else cout << "\033[0m";
        cout << flush;
    #endif
}

void drawHorizontalLine(int width, int color) {
    setColor(color);
    cout << "  +";
    for (int i = 0; i < width; i++) {
        cout << "-";
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    cout << "+\n";
    setColor(7);
}

// Prints the live HP bars after an action
void printLiveUpdate(string n1, int h1, int m1, string n2, int h2, int m2) {
    cout << "\n";
    setColor(11); cout << "  +------------------ LIVE STATUS ------------------+\n"; setColor(7);
    
    float r1 = (float)h1 / m1; int f1 = (int)(r1 * 15);
    cout << "  " << left << setw(15) << n1 << " [";
    if (r1 > 0.5) setColor(10); else if (r1 > 0.25) setColor(14); else setColor(12);
    for(int i=0; i<15; i++) cout << (i < f1 ? "|" : " ");
    setColor(7); cout << "] " << h1 << "/" << m1 << "\n";

    float r2 = (float)h2 / m2; int f2 = (int)(r2 * 15);
    cout << "  " << left << setw(15) << n2 << " [";
    if (r2 > 0.5) setColor(10); else if (r2 > 0.25) setColor(14); else setColor(12);
    for(int i=0; i<15; i++) cout << (i < f2 ? "|" : " ");
    setColor(7); cout << "] " << h2 << "/" << m2 << "\n";
    
    setColor(11); cout << "  +-------------------------------------------------+\n"; setColor(7);
    this_thread::sleep_for(chrono::milliseconds(1200));
}

// --- FILE MANAGEMENT ---
string getAccountFilename(string name) {
    for (char &c : name) c = tolower(c);
    return "accounts/acc_" + name + ".txt";
}

// Prevents crash if save file gets corrupted
int safeStoi(const string& value) {
    try { return stoi(value); }
    catch (...) { return 0; }
}

bool loadAccount(string name, PlayerAccount* acc) {
    string filename = getAccountFilename(name);
    ifstream inFile(filename);
    if (inFile.is_open()) {
        string lines[8];
        int lineCount = 0;
        while (lineCount < 8 && getline(inFile, lines[lineCount])) {
            if (!lines[lineCount].empty() && lines[lineCount].back() == '\r') lines[lineCount].pop_back();
            lineCount++;
        }
        inFile.close();

        if (lineCount == 8) {
            acc->username = lines[0]; acc->password = lines[1];
            acc->coins = safeStoi(lines[2]); acc->maxHp = safeStoi(lines[3]);
            acc->baseDamage = safeStoi(lines[4]); acc->overallLevel = safeStoi(lines[5]);
            acc->muscleLevel = safeStoi(lines[6]); acc->vitalityLevel = safeStoi(lines[7]);
            return true;
        }
    }
    return false;
}

void saveAccount(PlayerAccount* acc) {
    string filename = getAccountFilename(acc->username);
    ofstream outFile(filename);
    if (outFile.is_open()) {
        outFile << acc->username << "\n" << acc->password << "\n" << acc->coins << "\n" << acc->maxHp << "\n"
            << acc->baseDamage << "\n" << acc->overallLevel << "\n" << acc->muscleLevel << "\n" << acc->vitalityLevel << "\n";
        outFile.close();
    }
}

bool deleteAccountFile(string name) { return remove(getAccountFilename(name).c_str()) == 0; }
bool accountFileExists(const string& name) { ifstream file(getAccountFilename(name)); return file.is_open(); }

void manageAccountSystem(PlayerAccount* acc, bool* inHub) {
    bool inAccountMenu = true;
    while (inAccountMenu) {
        clearScreen();
        playSFX("scene");
        drawHorizontalLine(54, 13);
        setColor(13); cout << "                 ACCOUNT MANAGER                   \n";
        drawHorizontalLine(54, 13);
        setColor(7);
        this_thread::sleep_for(chrono::milliseconds(100));

        cout << "\n  Current Username: " << acc->username << "\n";
        this_thread::sleep_for(chrono::milliseconds(50));
        cout << "  Password Stored  : " << (acc->password.empty() ? "Legacy Account" : "Set") << "\n\n";
        this_thread::sleep_for(chrono::milliseconds(150));

        cout << "  1. Change Username\n";
        this_thread::sleep_for(chrono::milliseconds(60));
        cout << "  2. Change Password\n";
        this_thread::sleep_for(chrono::milliseconds(60));
        cout << "  3. Delete Account\n";
        this_thread::sleep_for(chrono::milliseconds(60));
        cout << "  4. Back\n";
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "  Choice: ";

        flushKeys();
        char choice = _getch();
        playSFX("key");

        if (choice == '1') {
            string newUsername = getInput("\n  Enter New Username: ");

            if (newUsername == acc->username) {
                setColor(14); cout << "  > New username is the same as the current one.\n"; setColor(7);
            }
            else if (accountFileExists(newUsername)) {
                setColor(12); cout << "  > That username already exists.\n"; setColor(7);
            }
            else {
                string oldUsername = acc->username;
                acc->username = newUsername;
                saveAccount(acc);
                remove(getAccountFilename(oldUsername).c_str());
                setColor(10); cout << "  > Username changed successfully.\n"; setColor(7);
            }
            this_thread::sleep_for(chrono::milliseconds(1200));
        }
        else if (choice == '2') {
            string newPassword = getInput("\n  Enter New Password: ");
            acc->password = newPassword;
            saveAccount(acc);
            setColor(10); cout << "  > Password updated successfully.\n"; setColor(7);
            this_thread::sleep_for(chrono::milliseconds(1200));
        }
        else if (choice == '3') {
            string confirmUsername = getInput("\n  Type your username to confirm deletion: ");
            string confirmPassword = getInput("  Type your password to confirm deletion: ");

            if (confirmUsername == acc->username && confirmPassword == acc->password) {
                if (deleteAccountFile(acc->username)) {
                    setColor(10); cout << "  > Account deleted successfully.\n"; setColor(7);
                    *inHub = false; inAccountMenu = false;
                }
                else { setColor(14); cout << "  > Account file not found.\n"; setColor(7); }
            }
            else { setColor(12); cout << "  > Confirmation failed. Account not deleted.\n"; setColor(7); }
            this_thread::sleep_for(chrono::milliseconds(1500));
        }
        else if (choice == '4') { inAccountMenu = false; }
    }
}

// --- ASCII ART & ANIMATIONS ---
void showTitleArt() {
    setColor(14);
    const char* art[] = {
        R"ART(   ____        _   _   _             __  ____  _          )ART",
        R"ART(  |  _ \      | | | | | |           / _||  _ \(_)         )ART",
        R"ART(  | |_) | __ _| |_| |_| | ___   ___| |_ | | | |_  ___ ___ )ART",
        R"ART(  |  _ < / _` | __| __| |/ _ \ / _ \  _|| | | | |/ __/ _ \)ART",
        R"ART(  | |_) | (_| | |_| |_| |  __/| (_) | | | |_| | | (_|  __/)ART",
        R"ART(  |____/ \__,_|\__|\__|_|\___| \___/|_| |____/|_|\___\___|)ART"
    };
    const int ART_SIZE = 6;
    for (int i = 0; i < ART_SIZE; i++) {
        cout << art[i] << "\n";
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    setColor(7);
}

void showTrophy(string winnerName) {
    setColor(14);
    const char* art[] = {
        R"ART(             ___________         )ART",
        R"ART(            '._==_==_=_.'        )ART",
        R"ART(            .-\:      /-.        )ART",
        R"ART(           | (|:.     |) |       )ART",
        R"ART(            '-|:.     |-'        )ART",
        R"ART(              \::.    /          )ART",
        R"ART(               '::. .'           )ART",
        R"ART(                 ) (             )ART",
        R"ART(               _.' '._           )ART",
        R"ART(              `"""""""`          )ART"
    };
    const int TROPHY_SIZE = 10;
    for (int i = 0; i < TROPHY_SIZE; i++) {
        cout << art[i] << "\n";
        this_thread::sleep_for(chrono::milliseconds(60));
    }
    setColor(10);
    cout << "        CHAMPION: ";
    for (char c : winnerName) {
        cout << c << flush;
        this_thread::sleep_for(chrono::milliseconds(80));
    }
    cout << "!\n\n";
    setColor(7);
}

void animateLoadingScreen() {
    clearScreen();
    playSFX("scene");
    int barLength = 40, startX = 10, startY = 5;
    gotoxy(startX, startY + 3);
    for (int i = 0; i < barLength + 5; i++) cout << "_";

    for (int i = 0; i <= barLength; i++) {
        int frame = i % 2;
        setColor(11);
        gotoxy(startX + i, startY);     cout << "  O  ";
        gotoxy(startX + i, startY + 1); cout << " /|\\ ";
        gotoxy(startX + i, startY + 2);
        if (frame == 0) cout << " / \\ "; else cout << "  |  ";
        gotoxy(startX, startY + 5);
        cout << "Loading Arena: " << (i * 100) / barLength << "% ";
        setColor(7);
        this_thread::sleep_for(chrono::milliseconds(170));
        if (i < barLength) {
            gotoxy(startX + i, startY);     cout << "     ";
            gotoxy(startX + i, startY + 1); cout << "     ";
            gotoxy(startX + i, startY + 2); cout << "     ";
        }
    }
    gotoxy(startX, startY + 7);
    cout << "System Ready! Press any key...";
    flushKeys(); (void)_getch();
}

// --- BATTLE MECHANICS ---
void displayStatus(string names[], int hp[], int maxHp[], int* invP1, int* invP2) {
    for (int p = 0; p < 2; p++) {
        int barWidth = 25;
        float ratio = (float)hp[p] / maxHp[p];
        int filled = (int)(ratio * barWidth);
        cout << "  " << left << setw(18) << names[p] << " ";
        if (ratio > 0.5) setColor(10); else if (ratio > 0.25) setColor(14); else setColor(12);
        cout << "[";
        for (int i = 0; i < barWidth; i++) {
            if (i < filled) { cout << "#"; this_thread::sleep_for(chrono::milliseconds(20)); }
            else { cout << "-"; this_thread::sleep_for(chrono::milliseconds(10)); }
        }
        cout << "] " << setw(3) << hp[p] << " / " << setw(3) << maxHp[p] << "\n";
        setColor(7);
        setColor(11);
        if (p == 0) { cout << "  > Inv: [" << invP1[0] << " Potions | " << invP1[1] << " Shields]\n\n"; this_thread::sleep_for(chrono::milliseconds(150)); }
        else { cout << "  > Inv: [" << invP2[0] << " Potions | " << invP2[1] << " Shields]\n\n"; }
        setColor(7);
    }
}

string actionNameFromDice(int diceRoll) {
    switch (diceRoll) {
    case 1: return "ATTACK"; case 2: return "DEFEND"; case 3: return "ITEM";
    case 4: return "CHARGE"; case 5: return "SPECIAL"; case 6: return "COMBO";
    default: return "ATTACK";
    }
}

int rollDice() {
    int result = 0;
    cout << "  Rolling Action Dice: [ ";
    int delay = 20;
    for (int i = 0; i < 15; i++) {
        result = (rand() % 6) + 1;
        cout << result << flush;
        cout << " ]";
        playSFX("tick");
        this_thread::sleep_for(chrono::milliseconds(delay));
        delay += 5;
        cout << "\b\b\b";
    }
    setColor(14); cout << result; setColor(7);
    cout << "\n";
    this_thread::sleep_for(chrono::milliseconds(300));
    return result;
}

int applyDefenseReduction(int damage, int defenseState) {
    if (defenseState == 1) return damage / 2;
    if (defenseState == 2) return damage / 10;
    return damage;
}

char chooseAttackStyle(bool isHuman, const string& attackerName) {
    if (!isHuman) {
        int r = rand() % 100;
        if (r < 60) return '2'; if (r < 85) return '1'; if (r < 95) return '3'; return '4';
    }
    setColor(11);
    cout << "  +-------------------------------------------------------------+\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  | CHOOSE ATTACK ARSENAL                                       |\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  +-------------------------------------------------------------+\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  | 1. Quick Strike     (Pro: Never Miss | Con: 1.0x Dmg Base)  |\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  | 2. Heavy Smash      (Pro: 1.5x Dmg   | Con: 33% Miss Chance)|\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  | 3. Precision Pierce (Pro: 2.0x Crit  | Con: Miss & 5 Recoil)|\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  | 4. Reckless Cleave  (Pro: Max 2.5x   | Con: Miss & 15 Recoil|\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  +-------------------------------------------------------------+\n";
    setColor(7);
    cout << "  Choice: ";
    flushKeys();
    char choice = _getch();
    playSFX("key");
    cout << choice << "\n";
    if (choice < '1' || choice > '4') return '1';
    return choice;
}

int chooseShieldStyle(bool isHuman, const string& defenderName, int* inventory, int diceRoll) {
    if (inventory[1] <= 0) return 0;
    if (!isHuman) {
        if (inventory[1] >= 2 && rand() % 100 < 30) {
            if (diceRoll <= 2) { inventory[1]--; setColor(14); cout << "  > Enemy Energy Shield faltered, so it fell back to a Standard Guard.\n"; setColor(7); return 1; }
            inventory[1] -= 2; setColor(13); cout << "  > Enemy deployed Energy Shield! Incoming damage reduced by 90%.\n"; setColor(7); return 2;
        }
        inventory[1]--; setColor(13); cout << "  > Enemy uses a normal Guard. Damage halved.\n"; setColor(7); return 1;
    }
    setColor(11);
    cout << "  +-------------------------------------------------------------+\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  | CHOOSE SHIELD TYPE                                          |\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  +-------------------------------------------------------------+\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  | 1. Standard Guard   (Pro: Reliable -50% Dmg | Con: Cost 1)  |\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  | 2. Overcharge Shield(Pro: Massive -90% Dmg  | Con: Cost 2* )|\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  |    *Warning: Fails completely if dice rolls 1 or 2.         |\n";
    this_thread::sleep_for(chrono::milliseconds(20));
    cout << "  +-------------------------------------------------------------+\n";
    setColor(7);
    cout << "  Choice: ";
    flushKeys();
    char choice = _getch();
    playSFX("key");
    cout << choice << "\n";

    if (choice == '2' && inventory[1] >= 2) {
        if (diceRoll <= 2) { inventory[1]--; setColor(14); cout << "  > OVERCHARGE MISFIRED! It fell back to a Standard Guard instead.\n"; setColor(7); return 1; }
        inventory[1] -= 2; setColor(11); cout << "  > OVERCHARGE DEPLOYED! Incoming damage reduced by 90%.\n"; setColor(7); return 2;
    }
    if (choice == '2' && inventory[1] == 1) { setColor(14); cout << "  > Not enough shields for Overcharge, using Standard Guard.\n"; setColor(7); inventory[1]--; return 1; }
    if (choice == '1' && inventory[1] > 0) { inventory[1]--; setColor(11); cout << "  > Standard Guard active. Damage halved.\n"; setColor(7); return 1; }
    return 0;
}

int resolveAttackDamage(int baseDmg, char attackStyle, int diceRoll, int& selfHp, bool criticalHit) {
    if (attackStyle == '2') {
        if (diceRoll <= 2) { setColor(12); cout << "  > Heavy Smash swung wide and missed!\n"; setColor(7); return 0; }
        setColor(13); cout << "  > Heavy Smash crushes the defense!\n"; setColor(7);
        int damage = (baseDmg * 3) / 2 + diceRoll;
        if (criticalHit) {
            playSFX("crit");
            setColor(14); cout << "  > CHARGE POWER! Critical Hit doubles the strike!\n"; setColor(7); damage *= 2;
        }
        return damage;
    }
    else if (attackStyle == '3') {
        if (diceRoll <= 3) { setColor(12); cout << "  > Precision Pierce missed the vital point! You stumbled (-5 HP).\n"; setColor(7); selfHp -= 5; return 0; }
        setColor(14); cout << "  > CRITICAL! Precision Pierce hits a vital artery!\n"; setColor(7);
        int damage = baseDmg * 2;
        if (criticalHit) {
            playSFX("crit");
            setColor(14); cout << "  > CHARGE POWER! Critical Hit doubles the strike!\n"; setColor(7); damage *= 2;
        }
        return damage;
    }
    else if (attackStyle == '4') {
        if (diceRoll <= 1) { setColor(12); cout << "  > Reckless Cleave completely misses! (-15 HP Recoil)\n"; setColor(7); selfHp -= 15; return 0; }
        setColor(12); cout << "  > Brutal Cleave lands! But the strain hurts you (-15 HP Recoil)\n"; setColor(7);
        selfHp -= 15;
        int damage = (baseDmg * 5) / 2;
        if (criticalHit) {
            playSFX("crit");
            setColor(14); cout << "  > CHARGE POWER! Critical Hit doubles the strike!\n"; setColor(7); damage *= 2;
        }
        return damage;
    }
    setColor(11); cout << "  > Quick Strike hits securely.\n"; setColor(7);
    int damage = baseDmg;
    if (criticalHit && damage > 0) {
        playSFX("crit");
        setColor(14); cout << "  > CHARGE POWER! Critical Hit doubles the strike!\n"; setColor(7); damage *= 2;
    }
    return damage;
}

int useItem(int* inventory, bool isHuman, int maxHp) {
    if (inventory[0] > 0) {
        if (isHuman) {
            setColor(11); cout << "  > You have " << inventory[0] << " Potions. Drink one? (Y/N): "; setColor(7);
            flushKeys();
            char choice = _getch();
            playSFX("key");
            cout << choice << "\n";
            this_thread::sleep_for(chrono::milliseconds(300));
            if (choice == 'y' || choice == 'Y') {
                inventory[0]--; int healAmount = maxHp / 4; if (healAmount < 1) healAmount = 1;
                setColor(10); cout << "  > Drank a Potion! Restored " << healAmount << " HP.\n"; setColor(7); return healAmount;
            }
            else { setColor(14); cout << "  > You saved your potion. Action skipped!\n"; setColor(7); return 0; }
        }
        else {
            inventory[0]--; int healAmount = maxHp / 4; if (healAmount < 1) healAmount = 1;
            setColor(10); cout << "  > Enemy Drank a Potion! Restored " << healAmount << " HP.\n"; setColor(7); return healAmount;
        }
    }
    setColor(12); cout << "  > Inventory empty! Turn wasted.\n"; setColor(7); return 0;
}

// --- MENUS & BRIEFINGS ---
void showPreMatchBriefing(int gameMode) {
    clearScreen();
    playSFX("scene");
    setColor(11);
    drawHorizontalLine(54, 11);
    this_thread::sleep_for(chrono::milliseconds(100));
    cout << "                BATTLE PROTOCOLS ENGAGED                \n";
    drawHorizontalLine(54, 11);
    this_thread::sleep_for(chrono::milliseconds(200));
    cout << "\n"; setColor(7);

    if (gameMode == 1) {
        cout << "  [ CLASSIC STRATEGY MODE ]\n";
        this_thread::sleep_for(chrono::milliseconds(150));
        cout << "  Roll the Strategy Dice to unlock a specific set of tactical actions.\n";
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "  Choose wisely from the options granted by the roll!\n\n";
        this_thread::sleep_for(chrono::milliseconds(200));
        setColor(12); cout << "    Roll 1 : "; setColor(7); cout << "Offensive / Defensive Options\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        setColor(11); cout << "    Roll 2 : "; setColor(7); cout << "Defensive / Survival Options\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        setColor(10); cout << "    Roll 3 : "; setColor(7); cout << "Survival / Recovery Options\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        setColor(14); cout << "    Roll 4 : "; setColor(7); cout << "Recovery / Power-Up Options\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        setColor(13); cout << "    Roll 5 : "; setColor(7); cout << "Power-Up / Ultimate Options\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        setColor(11); cout << "    Roll 6 : "; setColor(7); cout << "Wildcard (Attack / Defend / Combo)\n\n";
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    else {
        cout << "  [ DICE-BOUND MODE ]\n";
        this_thread::sleep_for(chrono::milliseconds(150));
        cout << "  Fate binds your hands! The dice strictly dictates your action.\n\n";
        this_thread::sleep_for(chrono::milliseconds(200));
        setColor(12); cout << "    Roll 1 : "; setColor(7); cout << "FORCED ATTACK\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        setColor(11); cout << "    Roll 2 : "; setColor(7); cout << "FORCED DEFEND\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        setColor(10); cout << "    Roll 3 : "; setColor(7); cout << "FORCED ITEM\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        setColor(14); cout << "    Roll 4 : "; setColor(7); cout << "FORCED CHARGE\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        setColor(13); cout << "    Roll 5 : "; setColor(7); cout << "FORCED SPECIAL\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        setColor(11); cout << "    Roll 6 : "; setColor(7); cout << "FORCED COMBO\n\n";
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    setColor(12); cout << "  [!] HAZARDS : "; setColor(7); cout << "Dynamic Arena Events trigger every 3 rounds!\n\n";
    this_thread::sleep_for(chrono::milliseconds(300));
    drawHorizontalLine(54, 11);
    this_thread::sleep_for(chrono::milliseconds(150));
    cout << "    Press any key to enter the Arena...";
    flushKeys(); (void)_getch();
}

void showLeaderboard() {
    clearScreen();
    playSFX("scene");
    drawHorizontalLine(48, 14);
    setColor(14); cout << "                 HALL OF CHAMPIONS                  \n";
    drawHorizontalLine(48, 14);
    cout << "\n"; setColor(7);
    ifstream file("leaderboard.txt");
    string winner, loser; int wins; bool found = false;
    while (file >> winner >> loser >> wins) {
        cout << left << "    [CHAMPION] " << setw(12) << winner << " (vs " << setw(12) << loser << ") | Total Wins: " << wins << "\n";
        this_thread::sleep_for(chrono::milliseconds(80)); found = true;
    }
    if (!found) cout << "    No champions yet. Be the first to claim glory!\n";
    file.close(); cout << "\n"; drawHorizontalLine(48, 14);
    cout << "  Press any key to return..."; flushKeys(); (void)_getch();
}

void manageDataSystem() {
    clearScreen();
    playSFX("scene");
    setColor(12); cout << "\n  [ SYSTEM OVERRIDE ]\n"; setColor(7);
    cout << "  Warning: You are accessing secure data.\n";
    cout << "  Type 'delete' to proceed, or anything else to cancel: ";
    flushKeys(); string command; cin >> command; cin.ignore(1000, '\n'); playSFX("enter");
    if (command == "delete") {
        cout << "\n  Select target for deletion:\n  1. Purge Leaderboard\n  2. Purge Match Logs\n  3. Purge ALL Data\n  Choice: ";
        flushKeys(); char delChoice; cin >> delChoice; cin.ignore(1000, '\n'); playSFX("enter");
        if (delChoice == '1' || delChoice == '3') {
            if (remove("leaderboard.txt") == 0) { setColor(10); cout << "\n  > Leaderboard successfully deleted."; setColor(7); }
            else { setColor(14); cout << "\n  > Leaderboard file not found."; setColor(7); }
        }
        if (delChoice == '2' || delChoice == '3') {
            if (remove("match_logs.txt") == 0) { setColor(10); cout << "\n  > Match Logs successfully deleted."; setColor(7); }
            else { setColor(14); cout << "\n  > Match Logs file not found."; setColor(7); }
        }
    }
    else { setColor(14); cout << "\n  > Operation cancelled."; setColor(7); }
    cin.ignore(1000, '\n'); cout << "\n\n  Press any key to return to menu..."; flushKeys(); (void)_getch();
}

void upgradeArea(PlayerAccount* acc) {
    bool inUpgrade = true;
    while (inUpgrade) {
        clearScreen();
        playSFX("scene");
        drawHorizontalLine(54, 13);
        setColor(13); cout << "                  STORY UPGRADE AREA                  \n";
        drawHorizontalLine(54, 13); setColor(7);
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "\n  +----------------------------------------------------+\n";
        this_thread::sleep_for(chrono::milliseconds(50));
        cout << "  | Gladiator   : " << left << setw(14) << acc->username << " | Coins : " << setw(11) << acc->coins << " |\n";
        this_thread::sleep_for(chrono::milliseconds(50));
        cout << "  | Overall Lvl : " << left << setw(14) << acc->overallLevel << " |                             |\n";
        this_thread::sleep_for(chrono::milliseconds(50));
        cout << "  +----------------------------------------------------+\n";
        setColor(12);
        cout << "  | Muscle Lvl  : " << left << setw(14) << acc->muscleLevel << " | Base DMG : " << setw(8) << acc->baseDamage << " |\n";
        this_thread::sleep_for(chrono::milliseconds(50));
        setColor(10);
        cout << "  | Vitality Lvl: " << left << setw(14) << acc->vitalityLevel << " | Max HP   : " << setw(8) << acc->maxHp << " |\n";
        this_thread::sleep_for(chrono::milliseconds(50));
        setColor(7);
        cout << "  +----------------------------------------------------+\n\n";
        this_thread::sleep_for(chrono::milliseconds(150));
        cout << "  1. Train Muscles (+5 Base Damage) - 50 Coins\n";
        this_thread::sleep_for(chrono::milliseconds(60));
        cout << "  2. Enhance Vitality (+20 Max HP) - 50 Coins\n";
        this_thread::sleep_for(chrono::milliseconds(60));
        cout << "  3. Return to Hub\n";
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "  Choice: ";
        flushKeys(); char choice = _getch(); playSFX("key");
        if (choice == '1') {
            if (acc->coins >= 50) { acc->coins -= 50; acc->baseDamage += 5; acc->overallLevel++; acc->muscleLevel++; setColor(12); cout << "\n  > Muscles Strengthened! Base Damage is now " << acc->baseDamage << "!\n"; setColor(7); }
            else { setColor(12); cout << "\n  > Not enough coins!\n"; setColor(7); }
        }
        else if (choice == '2') {
            if (acc->coins >= 50) { acc->coins -= 50; acc->maxHp += 20; acc->overallLevel++; acc->vitalityLevel++; setColor(10); cout << "\n  > Vitality Enhanced! Max HP is now " << acc->maxHp << "!\n"; setColor(7); }
            else { setColor(12); cout << "\n  > Not enough coins!\n"; setColor(7); }
        }
        else if (choice == '3') { inUpgrade = false; }
        saveAccount(acc);
        if (inUpgrade) { this_thread::sleep_for(chrono::milliseconds(1000)); }
    }
}

// --- CORE BATTLE ENGINE ---
void startBattle(PlayerAccount* p1Acc, PlayerAccount* p2Acc, bool isPvP, string specialMoves[], int gameMode) {
    showPreMatchBriefing(gameMode);
    string names[2] = { p1Acc->username, p2Acc->username };
    int hp[2] = { p1Acc->maxHp, p2Acc->maxHp };
    int maxHp[2] = { p1Acc->maxHp, p2Acc->maxHp };
    int baseDamage[2] = { p1Acc->baseDamage, p2Acc->baseDamage };
    int* invP1 = new int[2]; invP1[0] = 3; invP1[1] = 2;
    int* invP2 = new int[2]; invP2[0] = 3; invP2[1] = 2;
    int currentTurn = 0, p1Def = 0, p2Def = 0, lastDice1 = 0, lastDice2 = 0;
    int chargeReady[2] = { 0, 0 };
    srand((unsigned int)time(0));
    int initiativeCoin = rand() % 2;
    setColor(14);
    cout << "\n\n  [INITIATIVE] Flipping coin...";
    this_thread::sleep_for(chrono::milliseconds(800));
    cout << "\r  [INITIATIVE] It lands on...   ";
    this_thread::sleep_for(chrono::milliseconds(800));
    if (initiativeCoin == 0) cout << "\r  [INITIATIVE] HEADS! " << names[0] << " starts.      \n";
    else cout << "\r  [INITIATIVE] TAILS! " << names[1] << " starts.      \n";
    setColor(7);
    this_thread::sleep_for(chrono::milliseconds(1000));

    while (hp[0] > 0 && hp[1] > 0 && currentTurn < 50) {
        clearScreen();
        drawHorizontalLine(54, 11);
        displayStatus(names, hp, maxHp, invP1, invP2);
        drawHorizontalLine(54, 11);
        setColor(14); cout << "\n  === ROUND " << currentTurn + 1 << " ===\n"; setColor(7);

        if (currentTurn > 0 && (currentTurn + 1) % 3 == 0) {
            setColor(13); cout << "\n  [!!!] ARENA EVENT DETECTED [!!!]\n"; setColor(7);
            this_thread::sleep_for(chrono::milliseconds(800));
            if (rand() % 2 == 0) { setColor(12); cout << "  > ACID RAIN FALLS! Both combatants lose 10 HP.\n\n"; setColor(7); hp[0] -= 10; hp[1] -= 10; }
            else { setColor(10); cout << "  > HEALING WIND BLOWS! Both combatants gain 10 HP.\n\n"; setColor(7); hp[0] += 10; if (hp[0] > maxHp[0]) hp[0] = maxHp[0]; hp[1] += 10; if (hp[1] > maxHp[1]) hp[1] = maxHp[1]; }
            printLiveUpdate(names[0], hp[0], maxHp[0], names[1], hp[1], maxHp[1]);
            if (hp[0] <= 0 || hp[1] <= 0) break;
        }

        int p1DmgDealt = 0, p2DmgDealt = 0;
        
        // Lambda to handle a single turn (handles both human and AI)
        auto resolveTurn = [&](int actorIndex, int opponentIndex, int& actorDef, int& opponentDef,
            int* inventory, int& actorHp, int& opponentHp, int baseDmg,
            int& lastDice, const string& specialMove, bool isHuman, int& damageOut) {
                damageOut = 0; int dice = 0; char finalAction = '1';
                if (gameMode == 1) {
                    cout << "\n  [" << names[actorIndex] << "]";
                    if (lastDice == 5) cout << " [COMBO READY]";
                    if (chargeReady[actorIndex] > 0) cout << " [CRITICAL READY]";
                    cout << " Press any key to roll the Strategy Dice...";
                    if (isHuman) { flushKeys(); (void)_getch(); playSFX("key"); }
                    cout << "\n"; dice = rollDice();
                    string validActions = ""; setColor(13);
                    cout << "  [!] The Strategy Dice grants a specific set of tactical options:\n"; setColor(7);
                    if (dice == 1) { validActions = "12"; cout << "      > [1] Attack | [2] Defend\n"; }
                    else if (dice == 2) { validActions = "23"; cout << "      > [2] Defend | [3] Item\n"; }
                    else if (dice == 3) { validActions = "34"; cout << "      > [3] Item   | [4] Charge\n"; }
                    else if (dice == 4) { validActions = "45"; cout << "      > [4] Charge | [5] Special\n"; }
                    else if (dice == 5) { validActions = "56"; cout << "      > [5] Special| [6] Combo\n"; }
                    else if (dice == 6) { validActions = "126"; cout << "      > [1] Attack | [2] Defend | [6] Combo (Wildcard!)\n"; }
                    if (lastDice == 5) { setColor(14); cout << "  [!] COMBO READY! (Option 6 available if granted)\n"; setColor(7); }
                    if (chargeReady[actorIndex] > 0) { setColor(13); cout << "  [!] CRITICAL CHARGE ACTIVE!\n"; setColor(7); }
                    if (isHuman) {
                        cout << "  > Choose your action: "; flushKeys();
                        while (true) { finalAction = _getch(); if (validActions.find(finalAction) != string::npos) { cout << finalAction << "\n"; playSFX("key"); break; } }
                    }
                    else {
                        // Smart AI logic
                        if (validActions.find('3') != string::npos && actorHp < maxHp[actorIndex] * 0.35 && inventory[0] > 0) finalAction = '3';
                        else if (validActions.find('1') != string::npos && opponentHp <= baseDmg) finalAction = '1';
                        else if (validActions.find('6') != string::npos && lastDice == 5) finalAction = '6';
                        else if (validActions.find('4') != string::npos && actorHp > maxHp[actorIndex] * 0.5) finalAction = '4';
                        else if (validActions.find('2') != string::npos && inventory[1] > 0 && actorHp < maxHp[actorIndex] * 0.5) finalAction = '2';
                        else if (validActions.find('1') != string::npos) finalAction = '1';
                        else finalAction = validActions[0];
                        this_thread::sleep_for(chrono::milliseconds(800)); cout << "  Action: " << finalAction << "\n";
                    }
                }
                else {
                    cout << "\n  [" << names[actorIndex] << "] Press any key to roll the Dice of Binding...";
                    if (isHuman) { flushKeys(); (void)_getch(); playSFX("key"); }
                    cout << "\n"; dice = rollDice(); finalAction = '0' + dice;
                    setColor(12); cout << "  [!!!] FATE DECIDES! The Dice of Binding dictates your action!\n"; setColor(7);
                    cout << "      > Bound Action Forced: [" << actionNameFromDice(dice) << "]\n";
                    this_thread::sleep_for(chrono::milliseconds(800));
                }
                switch (finalAction) {
                case '1': {
                    char attackStyle = chooseAttackStyle(isHuman, names[actorIndex]);
                    setColor(11); cout << "  > " << names[actorIndex] << " launches an attack!\n"; setColor(7);
                    bool criticalHit = false;
                    if (chargeReady[actorIndex] > 0) { criticalHit = true; chargeReady[actorIndex] = 0; }
                    int hitRoll = (rand() % 6) + 1;
                    damageOut = resolveAttackDamage(baseDmg, attackStyle, hitRoll, actorHp, criticalHit); break;
                }
                case '2': actorDef = chooseShieldStyle(isHuman, names[actorIndex], inventory, (rand() % 6) + 1); break;
                case '3': actorHp += useItem(inventory, isHuman, maxHp[actorIndex]); if (actorHp > maxHp[actorIndex]) actorHp = maxHp[actorIndex]; break;
                case '4':
                    setColor(10); cout << "  > BATTLE CHARGE! " << names[actorIndex] << " regains 5 HP!\n"; setColor(7);
                    actorHp += 5; if (actorHp > maxHp[actorIndex]) actorHp = maxHp[actorIndex]; chargeReady[actorIndex] = 1;
                    setColor(14); cout << "  > NEXT ATTACK IS NOW A CRITICAL HIT!\n"; setColor(7); break;
                case '5': setColor(13); cout << "  > SPECIAL POWER UNLOCKED! Unleashing " << specialMove << "!!\n"; setColor(7); damageOut = baseDmg * 3; break;
                case '6':
                    if (lastDice == 5) { setColor(13); cout << "  > SYNERGY COMBO ACTIVATED!!! SUPER ULTIMATE UNLEASHED!\n"; setColor(7); damageOut = baseDmg * 4; }
                    else { setColor(14); cout << "  > COMBO STRIKE! " << specialMove << " hits rapidly!\n"; setColor(7); damageOut = baseDmg * 2; } break;
                }
                lastDice = dice;
                if (damageOut > 0) {
                    damageOut = applyDefenseReduction(damageOut, opponentDef);
                    setColor(12); cout << "  > " << names[actorIndex] << " dealt " << damageOut << " damage!\n"; setColor(7); opponentHp -= damageOut;
                }
                if (damageOut > 0 || finalAction == '3' || finalAction == '4') { printLiveUpdate(names[0], hp[0], maxHp[0], names[1], hp[1], maxHp[1]); }
                return dice;
            };

        p1Def = 0; p2Def = 0; int dice1 = 0, dice2 = 0;
        if (initiativeCoin == 0) {
            dice1 = resolveTurn(0, 1, p1Def, p2Def, invP1, hp[0], hp[1], baseDamage[0], lastDice1, specialMoves[0], true, p1DmgDealt);
            this_thread::sleep_for(chrono::milliseconds(800));
            if (hp[1] <= 0 || hp[0] <= 0) break;
            dice2 = resolveTurn(1, 0, p2Def, p1Def, invP2, hp[1], hp[0], baseDamage[1], lastDice2, specialMoves[1], isPvP, p2DmgDealt);
        }
        else {
            dice2 = resolveTurn(1, 0, p2Def, p1Def, invP2, hp[1], hp[0], baseDamage[1], lastDice2, specialMoves[1], isPvP, p2DmgDealt);
            this_thread::sleep_for(chrono::milliseconds(800));
            if (hp[1] <= 0 || hp[0] <= 0) break;
            dice1 = resolveTurn(0, 1, p1Def, p2Def, invP1, hp[0], hp[1], baseDamage[0], lastDice1, specialMoves[0], true, p1DmgDealt);
        }
        currentTurn++;
        cout << "\n  Press any key for the next round..."; flushKeys(); (void)_getch(); playSFX("key");
    }

    clearScreen(); drawHorizontalLine(54, 11);
    string winnerName = "", loserName = "";
    if (hp[0] > 0 && hp[1] <= 0) { winnerName = names[0]; loserName = names[1]; showTrophy(winnerName); }
    else if (hp[1] > 0 && hp[0] <= 0) { winnerName = names[1]; loserName = names[0]; showTrophy(winnerName); }
    else { setColor(14); cout << "\n  DRAW! Both combatants have fallen to the dust!\n\n"; }
    drawHorizontalLine(54, 11); setColor(7);

    if (!isPvP) {
        if (hp[0] > 0 && hp[1] <= 0) { setColor(10); cout << "  > STORY VICTORY! You earned 50 Coins for upgrades.\n"; setColor(7); p1Acc->coins += 50; }
        else if (hp[1] > 0 && hp[0] <= 0) { setColor(12); cout << "  > DEFEAT! You salvaged 15 Coins for your efforts.\n"; setColor(7); p1Acc->coins += 15; }
        else { setColor(14); cout << "  > TIE! You salvaged 20 Coins.\n"; setColor(7); p1Acc->coins += 20; }
        saveAccount(p1Acc);
    }

    // Update leaderboard file
    if (winnerName != "") {
        string lbWinners[100], lbLosers[100]; 
        int lbWins[100], lbCount = 0;
        ifstream readLB("leaderboard.txt");
        while (readLB >> lbWinners[lbCount] >> lbLosers[lbCount] >> lbWins[lbCount] && lbCount < 100) lbCount++;
        readLB.close();
        bool found = false;
        for (int i = 0; i < lbCount; i++) {
            if (lbWinners[i] == winnerName && lbLosers[i] == loserName) { lbWins[i]++; found = true; break; }
        }
        if (!found) { lbWinners[lbCount] = winnerName; lbLosers[lbCount] = loserName; lbWins[lbCount] = 1; lbCount++; }
        ofstream writeLB("leaderboard.txt", ios::trunc);
        for (int i = 0; i < lbCount; i++) writeLB << lbWinners[i] << " " << lbLosers[i] << " " << lbWins[i] << "\n";
        writeLB.close();
    }
    delete[] invP1; delete[] invP2;
    cout << "\n  Press any key to return to the Hub..."; flushKeys(); (void)_getch(); playSFX("key");
}

// --- MAIN PROGRAM LOOP ---
int main() {
    (void)CREATE_DIR("accounts");
    
    initAudio();
    playBGM();
    animateLoadingScreen();

    PlayerAccount currentAcc;
    bool systemRunning = true;
    while (systemRunning) {
        clearScreen();
        showTitleArt();
        drawHorizontalLine(54, 11);
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "  1. Login to Existing Account\n";
        this_thread::sleep_for(chrono::milliseconds(70));
        cout << "  2. Create New Account\n";
        this_thread::sleep_for(chrono::milliseconds(70));
        cout << "  3. Manage Account\n";
        this_thread::sleep_for(chrono::milliseconds(70));
        cout << "  4. Exit System\n";
        drawHorizontalLine(54, 11);
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "  Select an option: ";
        flushKeys(); char authChoice = _getch(); playSFX("key");

        if (authChoice == '4') { systemRunning = false; break; }
        if (authChoice == '1' || authChoice == '2') {
            string nameInput = getInput("\n\n  Enter Gladiator Name (No spaces): ");
            string passwordInput = getInput("  Enter Password: ");

            if (authChoice == '1') {
                if (!loadAccount(nameInput, &currentAcc)) { setColor(12); cout << "  > Account not found! Returning to menu...\n"; setColor(7); this_thread::sleep_for(chrono::milliseconds(1500)); continue; }
                if (currentAcc.password.empty()) { currentAcc.password = passwordInput; saveAccount(&currentAcc); setColor(14); cout << "  > Legacy account updated with a new password.\n"; setColor(7); }
                else if (currentAcc.password != passwordInput) { setColor(12); cout << "  > Wrong password! Returning to menu...\n"; setColor(7); this_thread::sleep_for(chrono::milliseconds(1500)); continue; }
                setColor(10); cout << "  > Login Successful! Welcome back, " << currentAcc.username << ".\n"; setColor(7);
            }
            else {
                if (loadAccount(nameInput, &currentAcc)) { setColor(12); cout << "  > Account already exists! Returning to menu...\n"; setColor(7); this_thread::sleep_for(chrono::milliseconds(1500)); continue; }
                currentAcc.username = nameInput; currentAcc.password = passwordInput; currentAcc.coins = 0; currentAcc.maxHp = 100; currentAcc.baseDamage = 15;
                currentAcc.overallLevel = 1; currentAcc.muscleLevel = 1; currentAcc.vitalityLevel = 1; saveAccount(&currentAcc);
                setColor(10); cout << "  > Account Created! Welcome to the Arena, " << currentAcc.username << ".\n"; setColor(7);
            }
            this_thread::sleep_for(chrono::milliseconds(1500));
            bool inHub = true;
            while (inHub) {
                clearScreen();
                playSFX("scene");
                drawHorizontalLine(54, 14);
                setColor(14); cout << "               [ GLADIATOR HUB: " << currentAcc.username << " ]\n"; setColor(7);
                drawHorizontalLine(54, 14);
                this_thread::sleep_for(chrono::milliseconds(100));
                cout << "\n  +----------------------------------------------------+\n";
                this_thread::sleep_for(chrono::milliseconds(50));
                cout << "  | Overall Lvl: " << left << setw(13) << currentAcc.overallLevel << " | Coins : " << setw(11) << currentAcc.coins << " |\n";
                this_thread::sleep_for(chrono::milliseconds(50));
                cout << "  | Max HP     : " << left << setw(13) << currentAcc.maxHp << " | DMG   : " << setw(11) << currentAcc.baseDamage << " |\n";
                this_thread::sleep_for(chrono::milliseconds(50));
                cout << "  +----------------------------------------------------+\n\n";
                this_thread::sleep_for(chrono::milliseconds(150));
                cout << "  1. Story Battle (Classic Strategy)\n";
                this_thread::sleep_for(chrono::milliseconds(50));
                cout << "  2. Story Battle (Dice-Bound Mode)\n";
                this_thread::sleep_for(chrono::milliseconds(50));
                cout << "  3. Local PvP (Classic Strategy)\n";
                this_thread::sleep_for(chrono::milliseconds(50));
                cout << "  4. Local PvP (Dice-Bound Mode)\n";
                this_thread::sleep_for(chrono::milliseconds(50));
                cout << "  5. Upgrade Area\n";
                this_thread::sleep_for(chrono::milliseconds(50));
                cout << "  6. View Hall of Champions\n";
                this_thread::sleep_for(chrono::milliseconds(50));
                cout << "  7. Manage Records (Delete Data)\n";
                this_thread::sleep_for(chrono::milliseconds(50));
                cout << "  8. Logout\n";
                cout << "\n";
                drawHorizontalLine(54, 11);
                this_thread::sleep_for(chrono::milliseconds(100));
                cout << "  Select an option: ";
                flushKeys(); char hubChoice = _getch(); playSFX("key");

                if (hubChoice == '1' || hubChoice == '2') {
                    int mode = (hubChoice == '1') ? 1 : 2;
                    PlayerAccount aiAcc = { "Mecha-Golem", "", 0, 150, 10 + (currentAcc.overallLevel * 2), 1, 1, 1 };
                    string spMoves[2] = { "OMNISLASH", "OBLITERATION_BEAM" };
                    startBattle(&currentAcc, &aiAcc, false, spMoves, mode);
                }
                else if (hubChoice == '3' || hubChoice == '4') {
                    int mode = (hubChoice == '3') ? 1 : 2;
                    string p2Name = getInput("\n\n  Enter Player 2 Name (Guest): ");
                    PlayerAccount p2Acc = { p2Name, "", 0, 100, 15, 1, 1, 1 };
                    string spMoves[2] = { "OMNISLASH", "METEOR_STRIKE" };
                    startBattle(&currentAcc, &p2Acc, true, spMoves, mode);
                }
                else if (hubChoice == '5') upgradeArea(&currentAcc);
                else if (hubChoice == '6') showLeaderboard();
                else if (hubChoice == '7') manageDataSystem();
                else if (hubChoice == '8') { saveAccount(&currentAcc); inHub = false; }
            }
        }
        else if (authChoice == '3') {
            string nameInput = getInput("\n\n  Enter Gladiator Name (No spaces): ");
            string passwordInput = getInput("  Enter Password: ");
            if (!loadAccount(nameInput, &currentAcc)) { setColor(12); cout << "  > Account not found! Returning to menu...\n"; setColor(7); this_thread::sleep_for(chrono::milliseconds(1500)); continue; }
            if (currentAcc.password.empty()) { currentAcc.password = passwordInput; saveAccount(&currentAcc); setColor(14); cout << "  > Legacy account updated with a new password.\n"; setColor(7); }
            else if (currentAcc.password != passwordInput) { setColor(12); cout << "  > Wrong password! Returning to menu...\n"; setColor(7); this_thread::sleep_for(chrono::milliseconds(1500)); continue; }
            bool manageOpen = true; manageAccountSystem(&currentAcc, &manageOpen);
        }
    }

    // Stop music and clean up audio engine
    stopBGM();
    cleanupAudio();

    clearScreen();
    cout << "\n  Shutting down system. Goodbye!\nMade by Zaid Ayyaz (0440), Abdul Hadi (0292), M.Haseeb (0462), Abdur Rehman (0297)\n\n";
#ifndef _WIN32
    cout << "\033[0m";
#endif
    return 0;
}