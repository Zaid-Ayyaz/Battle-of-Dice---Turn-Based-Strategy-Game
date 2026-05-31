#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <ctime>   
#include <cstdlib> 
#include <cstdio> 
#include <vector>

// ==========================================
// WINDOWS & LINUX SUPPORT
// ==========================================
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

// ==========================================
// ACCOUNT SYSTEM STRUCTURES
// ==========================================
struct PlayerAccount {
    string username;
    string password;
    int coins;
    int maxHp;
    int baseDamage;
    int overallLevel;
    int muscleLevel;
    int vitalityLevel;
};

// ==========================================
// UTILITY FUNCTIONS & DYNAMIC UI
// ==========================================

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
    for(int i = 0; i < width; i++) {
        cout << "-";
        this_thread::sleep_for(chrono::milliseconds(5));
    } 
    cout << "+\n";
    setColor(7);
}

// ==========================================
// ACCOUNT MANAGEMENT SYSTEM
// ==========================================

string getAccountFilename(string name) {
    string filenameCore = "";
    for (size_t i = 0; i < name.length(); i++) {
        if (name[i] != ' ') {
            char ch = name[i];
            if (ch >= 'A' && ch <= 'Z') ch = ch - 'A' + 'a';
            filenameCore += ch;
        }
        if (filenameCore.length() == 4) break;
    }
    return "accounts/acc_" + filenameCore + ".txt";
}

bool isIntegerLine(const string& value) {
    if (value.empty()) return false;
    size_t start = 0;
    if (value[0] == '-') start = 1;
    if (start >= value.length()) return false;
    for (size_t i = start; i < value.length(); i++) {
        if (value[i] < '0' || value[i] > '9') return false;
    }
    return true;
}

bool loadAccount(string name, PlayerAccount* acc) {
    string filename = getAccountFilename(name);
    ifstream inFile(filename);
    if (inFile.is_open()) {
        string lines[8];
        int lineCount = 0;
        while (lineCount < 8 && getline(inFile, lines[lineCount])) {
            if (!lines[lineCount].empty() && lines[lineCount].back() == '\r') {
                lines[lineCount].pop_back();
            }
            lineCount++;
        }
        inFile.close();
        if (lineCount == 7) {
            if (!isIntegerLine(lines[1]) || !isIntegerLine(lines[2]) || !isIntegerLine(lines[3]) ||
                !isIntegerLine(lines[4]) || !isIntegerLine(lines[5]) || !isIntegerLine(lines[6])) {
                return false;
            }
            acc->username = lines[0];
            acc->password = "";
            acc->coins = stoi(lines[1]);
            acc->maxHp = stoi(lines[2]);
            acc->baseDamage = stoi(lines[3]);
            acc->overallLevel = stoi(lines[4]);
            acc->muscleLevel = stoi(lines[5]);
            acc->vitalityLevel = stoi(lines[6]);
            return true;
        }

        if (lineCount == 8) {
            if (!isIntegerLine(lines[2]) || !isIntegerLine(lines[3]) || !isIntegerLine(lines[4]) ||
                !isIntegerLine(lines[5]) || !isIntegerLine(lines[6]) || !isIntegerLine(lines[7])) {
                return false;
            }
            acc->username = lines[0];
            acc->password = lines[1];
            acc->coins = stoi(lines[2]);
            acc->maxHp = stoi(lines[3]);
            acc->baseDamage = stoi(lines[4]);
            acc->overallLevel = stoi(lines[5]);
            acc->muscleLevel = stoi(lines[6]);
            acc->vitalityLevel = stoi(lines[7]);
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
                << acc->baseDamage << "\n" << acc->overallLevel << "\n" 
                << acc->muscleLevel << "\n" << acc->vitalityLevel << "\n";
        outFile.close();
    }
}

bool deleteAccountFile(string name) {
    string filename = getAccountFilename(name);
    return remove(filename.c_str()) == 0;
}

bool accountFileExists(const string& name) {
    ifstream file(getAccountFilename(name));
    return file.is_open();
}

void manageAccountSystem(PlayerAccount* acc, bool* inHub) {
    bool inAccountMenu = true;
    while (inAccountMenu) {
        clearScreen();
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

        if (choice == '1') {
            cout << "\n  Enter New Username: ";
            string newUsername;
            cin >> newUsername;
            cin.ignore(1000, '\n');

            if (newUsername == acc->username) {
                setColor(14); cout << "  > New username is the same as the current one.\n"; setColor(7);
            } else if (accountFileExists(newUsername)) {
                setColor(12); cout << "  > That username already exists.\n"; setColor(7);
            } else {
                string oldUsername = acc->username;
                acc->username = newUsername;
                saveAccount(acc);
                remove(getAccountFilename(oldUsername).c_str());
                setColor(10); cout << "  > Username changed successfully.\n"; setColor(7);
            }
            this_thread::sleep_for(chrono::milliseconds(1200));
        } else if (choice == '2') {
            cout << "\n  Enter New Password: ";
            string newPassword;
            cin >> newPassword;
            cin.ignore(1000, '\n');
            acc->password = newPassword;
            saveAccount(acc);
            setColor(10); cout << "  > Password updated successfully.\n"; setColor(7);
            this_thread::sleep_for(chrono::milliseconds(1200));
        } else if (choice == '3') {
            cout << "\n  Type your username to confirm deletion: ";
            string confirmUsername;
            cin >> confirmUsername;
            cout << "  Type your password to confirm deletion: ";
            string confirmPassword;
            cin >> confirmPassword;
            cin.ignore(1000, '\n');

            if (confirmUsername == acc->username && confirmPassword == acc->password) {
                if (deleteAccountFile(acc->username)) {
                    setColor(10); cout << "  > Account deleted successfully.\n"; setColor(7);
                    *inHub = false;
                    inAccountMenu = false;
                } else {
                    setColor(14); cout << "  > Account file not found.\n"; setColor(7);
                }
            } else {
                setColor(12); cout << "  > Confirmation failed. Account not deleted.\n"; setColor(7);
            }
            this_thread::sleep_for(chrono::milliseconds(1500));
        } else if (choice == '4') {
            inAccountMenu = false;
        }
    }
}

// ==========================================
// ASCII ART COMPONENTS (ANIMATED)
// ==========================================

void showTitleArt() {
    setColor(14); 

    /*     ____        _   _   _             __  ____  _          
          |  _ \      | | | | | |           / _||  _ \(_)         
          | |_) | __ _| |_| |_| | ___   ___| |_ | | | |_  ___ ___ 
          |  _ < / _` | __| __| |/ _ \ / _ \  _|| | | | |/ __/ _ \
          | |_) | (_| | |_| |_| |  __/| (_) | | | |_| | | (_|  __/
          |____/ \__,_|\__|\__|_|\___| \___/|_| |____/|_|\___\___|
        
    */

    // CUSTOM DELIMITER 'ART' prevents the ')' in the ASCII art from breaking the string
    const char* art[] = {
        R"ART(   ____        _   _   _             __  ____  _          )ART",
        R"ART(  |  _ \      | | | | | |           / _||  _ \(_)         )ART",
        R"ART(  | |_) | __ _| |_| |_| | ___   ___| |_ | | | |_  ___ ___ )ART",
        R"ART(  |  _ < / _` | __| __| |/ _ \ / _ \  _|| | | | |/ __/ _ \)ART",
        R"ART(  | |_) | (_| | |_| |_| |  __/| (_) | | | |_| | | (_|  __/)ART",
        R"ART(  |____/ \__,_|\__|\__|_|\___| \___/|_| |____/|_|\___\___|)ART"
    };

    // Hardcoded size bypasses debugger sizeof quirks
    const int ART_SIZE = 6;
    
    for (int i = 0; i < ART_SIZE; i++) {
        cout << art[i] << "\n";
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    setColor(7);
}

void showTrophy(string winnerName) {
    setColor(14); 

    /*
                     ___________
                    '._==_==_=_.'
                    .-\:      /-.
                   | (|:.     |) |
                    '-|:.     |-'
                      \::.    /
                       '::. .'
                         ) (
                       _.' '._
                      `"""""""`
    */

    const char* art[] = {
        R"(             ___________         )",
        R"(            '._==_==_=_.'        )",
        R"(            .-\:      /-.        )",
        R"(           | (|:.     |) |       )",
        R"(            '-|:.     |-'        )",
        R"(              \\::.    /         )",
        R"(               '::. .'           )",
        R"(                 ) (             )",
        R"(               _.' '._           )",
        R"(              `\"\"\"\"\"\"\"`   )"
    };
    // Animate assembling the trophy
    for (const char* line : art) {
        cout << line << "\n";
        this_thread::sleep_for(chrono::milliseconds(60));
    }
    setColor(10);
    cout << "        CHAMPION: ";
    // Typewriter effect for the winner's name
    for (char c : winnerName) {
        cout << c << flush;
        this_thread::sleep_for(chrono::milliseconds(80));
    }
    cout << "!\n\n";
    setColor(7);
}

void animateLoadingScreen() {
    clearScreen(); 
    int barLength = 40;  
    int startX = 10;     
    int startY = 5;      

    gotoxy(startX, startY + 3);
    for(int i = 0; i < barLength + 5; i++) cout << "_";

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
        
        this_thread::sleep_for(chrono::milliseconds(100)); 

        if (i < barLength) {
            gotoxy(startX + i, startY);     cout << "     ";
            gotoxy(startX + i, startY + 1); cout << "     ";
            gotoxy(startX + i, startY + 2); cout << "     ";
        }
    }
    gotoxy(startX, startY + 7);
    cout << "System Ready! Press any key..."; 
    flushKeys(); _getch();
}

// ==========================================
// BATTLE FUNCTIONS & MECHANICS
// ==========================================

void displayStatus(string names[], int hp[], int maxHp[], int* invP1, int* invP2) {
    for (int p = 0; p < 2; p++) {
        int barWidth = 25;
        float ratio = (float)hp[p] / maxHp[p];
        int filled = ratio * barWidth;

        cout << "  " << left << setw(18) << names[p] << " ";
        if (ratio > 0.5) setColor(10);      
        else if (ratio > 0.25) setColor(14); 
        else setColor(12);                  

        cout << "[";
        for (int i = 0; i < barWidth; i++) {
            if (i < filled) {
                cout << "#";
                this_thread::sleep_for(chrono::milliseconds(20));
            }
            else {
                cout << "-";
                this_thread::sleep_for(chrono::milliseconds(10));
            }
        }
        cout << "] " << setw(3) << hp[p] << " / " << setw(3) << maxHp[p] << "\n";
        setColor(7); 
        
        setColor(11);
        if (p == 0) {
            cout << "  > Inv: [" << invP1[0] << " Potions | " << invP1[1] << " Shields]\n\n";
            this_thread::sleep_for(chrono::milliseconds(150)); // Pause between players
        } else {
            cout << "  > Inv: [" << invP2[0] << " Potions | " << invP2[1] << " Shields]\n\n";
        }
        setColor(7);
    }
}

string actionNameFromDice(int diceRoll) {
    switch (diceRoll) {
        case 1: return "ATTACK";
        case 2: return "DEFEND";
        case 3: return "ITEM";
        case 4: return "CHARGE";
        case 5: return "SPECIAL";
        case 6: return "COMBO";
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
        int roll = rand() % 4;
        return char('1' + roll);
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
    cout << choice << "\n";
    if (choice < '1' || choice > '4') return '1';
    return choice;
}

int chooseShieldStyle(bool isHuman, const string& defenderName, int* inventory, int diceRoll) {
    if (inventory[1] <= 0) return 0;

    if (!isHuman) {
        int choice = (inventory[1] >= 2 && rand() % 2 == 0) ? 2 : 1;
        if (choice == 2) {
            if (diceRoll <= 2) {
                inventory[1]--;
                setColor(14); cout << "  > Enemy Energy Shield faltered, so it fell back to a Standard Guard.\n"; setColor(7);
                return 1;
            }
            inventory[1] -= 2;
            setColor(13); cout << "  > Enemy deployed Energy Shield! Incoming damage reduced by 90%.\n"; setColor(7);
            return 2;
        }
        inventory[1]--;
        setColor(13); cout << "  > Enemy uses a normal Guard. Damage halved.\n"; setColor(7);
        return 1;
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
    cout << choice << "\n";

    if (choice == '2' && inventory[1] >= 2) {
        if (diceRoll <= 2) {
            inventory[1]--;
            setColor(14); cout << "  > OVERCHARGE MISFIRED! It fell back to a Standard Guard instead.\n"; setColor(7);
            return 1;
        }
        inventory[1] -= 2;
        setColor(11); cout << "  > OVERCHARGE DEPLOYED! Incoming damage reduced by 90%.\n"; setColor(7);
        return 2;
    }

    if (choice == '2' && inventory[1] == 1) {
        setColor(14); cout << "  > Not enough shields for Overcharge, using Standard Guard.\n"; setColor(7);
        inventory[1]--;
        return 1;
    }

    if (choice == '1' && inventory[1] > 0) {
        inventory[1]--;
        setColor(11); cout << "  > Standard Guard active. Damage halved.\n"; setColor(7);
        return 1;
    }

    return 0; 
}

int resolveAttackDamage(int baseDmg, char attackStyle, int diceRoll, int& selfHp, bool criticalHit) {
    if (attackStyle == '2') { 
        if (diceRoll <= 2) { 
            setColor(12); cout << "  > Heavy Smash swung wide and missed!\n"; setColor(7);
            return 0;
        }
        setColor(13); cout << "  > Heavy Smash crushes the defense!\n"; setColor(7);
        int damage = (baseDmg * 3) / 2 + diceRoll;
        if (criticalHit) {
            setColor(14); cout << "  > CHARGE POWER! Critical Hit doubles the strike!\n"; setColor(7);
            damage *= 2;
        }
        return damage;
    } else if (attackStyle == '3') { 
        if (diceRoll <= 3) { 
            setColor(12); cout << "  > Precision Pierce missed the vital point! You stumbled (-5 HP).\n"; setColor(7);
            selfHp -= 5;
            return 0;
        }
        setColor(14); cout << "  > CRITICAL! Precision Pierce hits a vital artery!\n"; setColor(7);
        int damage = baseDmg * 2;
        if (criticalHit) {
            setColor(14); cout << "  > CHARGE POWER! Critical Hit doubles the strike!\n"; setColor(7);
            damage *= 2;
        }
        return damage;
    } else if (attackStyle == '4') { 
        if (diceRoll <= 1) { 
            setColor(12); cout << "  > Reckless Cleave completely misses! (-15 HP Recoil)\n"; setColor(7);
            selfHp -= 15;
            return 0;
        }
        setColor(12); cout << "  > Brutal Cleave lands! But the strain hurts you (-15 HP Recoil)\n"; setColor(7);
        selfHp -= 15;
        int damage = (baseDmg * 5) / 2;
        if (criticalHit) {
            setColor(14); cout << "  > CHARGE POWER! Critical Hit doubles the strike!\n"; setColor(7);
            damage *= 2;
        }
        return damage;
    }

    setColor(11); cout << "  > Quick Strike hits securely.\n"; setColor(7);
    int damage = baseDmg;
    if (criticalHit && damage > 0) {
        setColor(14); cout << "  > CHARGE POWER! Critical Hit doubles the strike!\n"; setColor(7);
        damage *= 2;
    }

    return damage;
}

int useItem(int* inventory, bool isHuman, int maxHp) {
    if (inventory[0] > 0) {
        if (isHuman) {
            setColor(11); cout << "  > You have " << inventory[0] << " Potions. Drink one? (Y/N): "; setColor(7);
            flushKeys(); 
            char choice = _getch(); cout << choice << "\n";
            this_thread::sleep_for(chrono::milliseconds(300));
            
            if (choice == 'y' || choice == 'Y') {
                inventory[0]--;
                int healAmount = maxHp / 4;
                if (healAmount < 1) healAmount = 1;
                setColor(10); cout << "  > Drank a Potion! Restored " << healAmount << " HP.\n"; setColor(7);
                return healAmount;
            } else {
                setColor(14); cout << "  > You saved your potion. Action skipped!\n"; setColor(7);
                return 0;
            }
        } else {
            inventory[0]--;
            int healAmount = maxHp / 4;
            if (healAmount < 1) healAmount = 1;
            setColor(10); cout << "  > Enemy Drank a Potion! Restored " << healAmount << " HP.\n"; setColor(7);
            return healAmount;
        }
    }
    setColor(12); cout << "  > Inventory empty! Turn wasted.\n"; setColor(7);
    return 0;
}

// ==========================================
// MENUS & BRIEFING (ANIMATED DRAW-IN)
// ==========================================

void showPreMatchBriefing(int gameMode) {
    clearScreen();
    setColor(11);
    drawHorizontalLine(54, 11);
    this_thread::sleep_for(chrono::milliseconds(100));
    cout << "                BATTLE PROTOCOLS ENGAGED                \n";
    drawHorizontalLine(54, 11);
    this_thread::sleep_for(chrono::milliseconds(200));
    cout << "\n";
    setColor(7);

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
    } else {
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
    flushKeys(); _getch();
}

void showLeaderboard() {
    clearScreen();
    drawHorizontalLine(48, 14);
    setColor(14); cout << "                 HALL OF CHAMPIONS                  \n";
    drawHorizontalLine(48, 14);
    cout << "\n";
    setColor(7);

    ifstream file("leaderboard.txt");
    string winner, loser; 
    int wins;
    bool found = false;
    
    while (file >> winner >> loser >> wins) {
        cout << left << "    [CHAMPION] " << setw(12) << winner 
             << " (vs " << setw(12) << loser << ") | Total Wins: " << wins << "\n";
        this_thread::sleep_for(chrono::milliseconds(80));
        found = true;
    }
    if (!found) cout << "    No champions yet. Be the first to claim glory!\n";
    
    file.close(); 
    cout << "\n";
    drawHorizontalLine(48, 14);
    cout << "  Press any key to return..."; 
    flushKeys(); _getch();
}

void manageDataSystem() {
    clearScreen();
    setColor(12);
    cout << "\n  [ SYSTEM OVERRIDE ]\n";
    setColor(7);
    cout << "  Warning: You are accessing secure data.\n";
    cout << "  Type 'delete' to proceed, or anything else to cancel: ";
    
    flushKeys();
    string command;
    cin >> command;
    cin.ignore(1000, '\n');
    
    if (command == "delete") {
        cout << "\n  Select target for deletion:\n";
        cout << "  1. Purge Leaderboard\n";
        cout << "  2. Purge Match Logs\n";
        cout << "  3. Purge ALL Data\n";
        cout << "  Choice: ";
        
        flushKeys();
        char delChoice;
        cin >> delChoice; 
        
        if (delChoice == '1' || delChoice == '3') {
            if (remove("leaderboard.txt") == 0) {
                setColor(10); cout << "\n  > Leaderboard successfully deleted."; setColor(7);
            } else {
                setColor(14); cout << "\n  > Leaderboard file not found."; setColor(7);
            }
        }
        if (delChoice == '2' || delChoice == '3') {
            if (remove("match_logs.txt") == 0) {
                setColor(10); cout << "\n  > Match Logs successfully deleted."; setColor(7);
            } else {
                setColor(14); cout << "\n  > Match Logs file not found."; setColor(7);
            }
        }
    } else {
        setColor(14); cout << "\n  > Operation cancelled."; setColor(7);
    }
    
    cin.ignore(1000, '\n'); 
    cout << "\n\n  Press any key to return to menu..."; 
    flushKeys(); _getch();
}

void upgradeArea(PlayerAccount* acc) {
    bool inUpgrade = true;
    while (inUpgrade) {
        clearScreen();
        drawHorizontalLine(54, 13);
        setColor(13); cout << "                  STORY UPGRADE AREA                  \n";
        drawHorizontalLine(54, 13);
        setColor(7);
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

        flushKeys();
        char choice = _getch();
        
        if (choice == '1') {
            if (acc->coins >= 50) {
                acc->coins -= 50;
                acc->baseDamage += 5;
                acc->overallLevel++;
                acc->muscleLevel++;
                setColor(12); cout << "\n  > Muscles Strengthened! Base Damage is now " << acc->baseDamage << "!\n"; setColor(7);
            } else {
                setColor(12); cout << "\n  > Not enough coins!\n"; setColor(7);
            }
        } else if (choice == '2') {
            if (acc->coins >= 50) {
                acc->coins -= 50;
                acc->maxHp += 20;
                acc->overallLevel++;
                acc->vitalityLevel++;
                setColor(10); cout << "\n  > Vitality Enhanced! Max HP is now " << acc->maxHp << "!\n"; setColor(7);
            } else {
                setColor(12); cout << "\n  > Not enough coins!\n"; setColor(7);
            }
        } else if (choice == '3') {
            inUpgrade = false;
        }

        saveAccount(acc);
        if(inUpgrade) {
            this_thread::sleep_for(chrono::milliseconds(1000));
        }
    }
}

// ==========================================
// CORE BATTLE ENGINE (PvP & AI Support)
// ==========================================

void startBattle(PlayerAccount* p1Acc, PlayerAccount* p2Acc, bool isPvP, string specialMoves[], int gameMode) {
    showPreMatchBriefing(gameMode); 

    string names[2] = {p1Acc->username, p2Acc->username};
    int hp[2] = {p1Acc->maxHp, p2Acc->maxHp};
    int maxHp[2] = {p1Acc->maxHp, p2Acc->maxHp};
    int baseDamage[2] = {p1Acc->baseDamage, p2Acc->baseDamage};

    int* invP1 = new int[2]; invP1[0] = 3; invP1[1] = 2; 
    int* invP2 = new int[2]; invP2[0] = 3; invP2[1] = 2; 

    int battleLog[50][3]; 
    for(int i=0; i<50; i++) { battleLog[i][0]=0; battleLog[i][1]=0; battleLog[i][2]=0; }
    
    int currentTurn = 0;
    int p1Def = 0, p2Def = 0;
    int lastDice1 = 0, lastDice2 = 0;
    int chargeReady[2] = {0, 0};
    int turnOrder[2] = {0, 1};

    srand(time(0));
    int initiativeCoin = rand() % 2;
    setColor(14);
    cout << "\n\n" << "  [INITIATIVE] Coin toss... ";
    if (initiativeCoin == 0) cout << "HEADS. " << names[0] << " gets the opening turn.\n";
    else cout << "TAILS. " << names[1] << " gets the opening turn.\n";
    setColor(7);
    this_thread::sleep_for(chrono::milliseconds(700));

    while (hp[0] > 0 && hp[1] > 0 && currentTurn < 50) {
        clearScreen();
        drawHorizontalLine(54, 11);
        displayStatus(names, hp, maxHp, invP1, invP2);
        drawHorizontalLine(54, 11);

        setColor(14); cout << "\n  === ROUND " << currentTurn + 1 << " ===\n"; setColor(7);

        if (currentTurn > 0 && (currentTurn + 1) % 3 == 0) {
            setColor(13); cout << "\n  [!!!] ARENA EVENT DETECTED [!!!]\n"; setColor(7);
            this_thread::sleep_for(chrono::milliseconds(800));
            
            if (rand() % 2 == 0) {
                setColor(12); cout << "  > ACID RAIN FALLS! Both combatants lose 10 HP.\n\n"; setColor(7);
                hp[0] -= 10; hp[1] -= 10;
                battleLog[currentTurn][2] = -10; 
            } else {
                setColor(10); cout << "  > HEALING WIND BLOWS! Both combatants gain 10 HP.\n\n"; setColor(7);
                hp[0] += 10; if (hp[0] > maxHp[0]) hp[0] = maxHp[0];
                hp[1] += 10; if (hp[1] > maxHp[1]) hp[1] = maxHp[1];
                battleLog[currentTurn][2] = 10; 
            }
            this_thread::sleep_for(chrono::milliseconds(1000));
            if (hp[0] <= 0 || hp[1] <= 0) break; 
        }
        
        int p1DmgDealt = 0;
        int p2DmgDealt = 0;

        auto resolveTurn = [&](int actorIndex, int opponentIndex, int& actorDef, int& opponentDef,
                               int* inventory, int& actorHp, int& opponentHp, int baseDmg,
                               int& lastDice, const string& specialMove, bool isHuman, int& damageOut) {
            
            damageOut = 0;
            int dice = 0;
            char finalAction = '1';

            if (gameMode == 1) { // CLASSIC STRATEGY (Subset Choice)
                cout << "\n  [" << names[actorIndex] << "]";
                if (lastDice == 5) cout << " [COMBO READY]";
                if (chargeReady[actorIndex] > 0) cout << " [CRITICAL READY]";
                cout << " Press any key to roll the Strategy Dice...";
                if(isHuman) { flushKeys(); _getch(); }
                cout << "\n";
                dice = rollDice();

                string validActions = "";
                setColor(13);
                cout << "  [!] The Strategy Dice grants a specific set of tactical options:\n";
                setColor(7);
                if (dice == 1)      { validActions = "12"; cout << "      > [1] Attack | [2] Defend\n"; }
                else if (dice == 2) { validActions = "23"; cout << "      > [2] Defend | [3] Item\n"; }
                else if (dice == 3) { validActions = "34"; cout << "      > [3] Item   | [4] Charge\n"; }
                else if (dice == 4) { validActions = "45"; cout << "      > [4] Charge | [5] Special\n"; }
                else if (dice == 5) { validActions = "56"; cout << "      > [5] Special| [6] Combo\n"; }
                else if (dice == 6) { validActions = "126";cout << "      > [1] Attack | [2] Defend | [6] Combo (Wildcard!)\n"; }

                if (lastDice == 5) {
                    setColor(14); cout << "  [!] COMBO READY! (Option 6 available if granted)\n"; setColor(7);
                }
                if (chargeReady[actorIndex] > 0) {
                    setColor(13); cout << "  [!] CRITICAL CHARGE ACTIVE!\n"; setColor(7);
                }

                if (isHuman) {
                    cout << "  > Choose your action: ";
                    flushKeys();
                    while (true) {
                        finalAction = _getch();
                        if (validActions.find(finalAction) != string::npos) {
                            cout << finalAction << "\n";
                            break;
                        }
                    }
                } else {
                    int aiRoll = rand() % 100;
                    if (validActions.find('6') != string::npos && lastDice == 5 && aiRoll < 60) finalAction = '6';
                    else if (validActions.find('5') != string::npos && aiRoll < 75) finalAction = '5';
                    else if (validActions.find('3') != string::npos && actorHp < maxHp[actorIndex] * 0.5 && inventory[0] > 0 && aiRoll < 85) finalAction = '3';
                    else if (validActions.find('4') != string::npos && aiRoll < 90) finalAction = '4';
                    else if (validActions.find('2') != string::npos && inventory[1] > 0 && aiRoll < 95) finalAction = '2';
                    else if (validActions.find('1') != string::npos) finalAction = '1';
                    else finalAction = validActions[0];
                    
                    this_thread::sleep_for(chrono::milliseconds(800));
                    cout << "  Action: " << finalAction << "\n";
                }
            } else { // DICE-BOUND MODE (Strict Binding)
                cout << "\n  [" << names[actorIndex] << "] Press any key to roll the Dice of Binding...";
                if(isHuman) { flushKeys(); _getch(); }
                cout << "\n";
                dice = rollDice();
                
                finalAction = '0' + dice; 
                
                setColor(12);
                cout << "  [!!!] FATE DECIDES! The Dice of Binding dictates your action!\n";
                setColor(7);
                cout << "      > Bound Action Forced: [" << actionNameFromDice(dice) << "]\n";
                this_thread::sleep_for(chrono::milliseconds(800));
            }

            // RESOLVE SELECTED ACTION
            switch (finalAction) {
                case '1':
                {
                    char attackStyle = chooseAttackStyle(isHuman, names[actorIndex]);
                    setColor(11); cout << "  > " << names[actorIndex] << " launches an attack!\n"; setColor(7);
                    bool criticalHit = false;
                    if (chargeReady[actorIndex] > 0) {
                        criticalHit = true;
                        chargeReady[actorIndex] = 0;
                    }
                    int hitRoll = (rand() % 6) + 1;
                    damageOut = resolveAttackDamage(baseDmg, attackStyle, hitRoll, actorHp, criticalHit);
                    break;
                }
                case '2':
                    actorDef = chooseShieldStyle(isHuman, names[actorIndex], inventory, (rand() % 6) + 1);
                    break;
                case '3':
                    actorHp += useItem(inventory, isHuman, maxHp[actorIndex]);
                    if (actorHp > maxHp[actorIndex]) actorHp = maxHp[actorIndex];
                    break;
                case '4':
                    setColor(10); cout << "  > BATTLE CHARGE! " << names[actorIndex] << " regains 5 HP!\n"; setColor(7);
                    actorHp += 5;
                    if (actorHp > maxHp[actorIndex]) actorHp = maxHp[actorIndex];
                    chargeReady[actorIndex] = 1;
                    setColor(14); cout << "  > NEXT ATTACK IS NOW A CRITICAL HIT!\n"; setColor(7);
                    break;
                case '5':
                    setColor(13); cout << "  > SPECIAL POWER UNLOCKED! Unleashing " << specialMove << "!!\n"; setColor(7);
                    damageOut = baseDmg * 3;
                    break;
                case '6':
                    if (lastDice == 5) {
                        setColor(13); cout << "  > SYNERGY COMBO ACTIVATED!!! SUPER ULTIMATE UNLEASHED!\n"; setColor(7);
                        damageOut = baseDmg * 4;
                    } else {
                        setColor(14); cout << "  > COMBO STRIKE! " << specialMove << " hits rapidly!\n"; setColor(7);
                        damageOut = baseDmg * 2; 
                    }
                    break;
            }

            lastDice = dice;

            if (damageOut > 0) {
                damageOut = applyDefenseReduction(damageOut, opponentDef);
                setColor(12); cout << "  > " << names[actorIndex] << " dealt " << damageOut << " damage!\n"; setColor(7);
                opponentHp -= damageOut;
            }

            return dice;
        };

        p1Def = 0;
        p2Def = 0;
        int dice1 = 0;
        int dice2 = 0;

        if (initiativeCoin == 0) {
            turnOrder[0] = 0;
            turnOrder[1] = 1;
            dice1 = resolveTurn(0, 1, p1Def, p2Def, invP1, hp[0], hp[1], baseDamage[0], lastDice1, specialMoves[0], true, p1DmgDealt);
            this_thread::sleep_for(chrono::milliseconds(800)); 
            if (hp[1] <= 0 || hp[0] <= 0) break; 
            dice2 = resolveTurn(1, 0, p2Def, p1Def, invP2, hp[1], hp[0], baseDamage[1], lastDice2, specialMoves[1], isPvP, p2DmgDealt);
        } else {
            turnOrder[0] = 1;
            turnOrder[1] = 0;
            dice2 = resolveTurn(1, 0, p2Def, p1Def, invP2, hp[1], hp[0], baseDamage[1], lastDice2, specialMoves[1], isPvP, p2DmgDealt);
            this_thread::sleep_for(chrono::milliseconds(800)); 
            if (hp[1] <= 0 || hp[0] <= 0) break; 
            dice1 = resolveTurn(0, 1, p1Def, p2Def, invP1, hp[0], hp[1], baseDamage[0], lastDice1, specialMoves[0], true, p1DmgDealt);
        }

        battleLog[currentTurn][0] = p1DmgDealt;
        battleLog[currentTurn][1] = p2DmgDealt;
        currentTurn++;

        cout << "\n  Press any key for the next round..."; 
        flushKeys(); _getch();
    }

    // --- BATTLE RESULTS ---
    clearScreen();
    drawHorizontalLine(54, 11);
    string winnerName = "", loserName = "";
    if (hp[0] > 0 && hp[1] <= 0) {
        winnerName = names[0]; loserName = names[1];
        showTrophy(winnerName);
    } else if (hp[1] > 0 && hp[0] <= 0) {
        winnerName = names[1]; loserName = names[0];
        showTrophy(winnerName);
    } else {
        setColor(14); cout << "\n  DRAW! Both combatants have fallen to the dust!\n\n";
    }
    drawHorizontalLine(54, 11);
    setColor(7);

    if (!isPvP) {
        if (hp[0] > 0 && hp[1] <= 0) {
            setColor(10); cout << "  > STORY VICTORY! You earned 50 Coins for upgrades.\n"; setColor(7);
            p1Acc->coins += 50;
        } else if (hp[1] > 0 && hp[0] <= 0) {
            setColor(12); cout << "  > DEFEAT! You salvaged 15 Coins for your efforts.\n"; setColor(7);
            p1Acc->coins += 15;
        } else {
            setColor(14); cout << "  > TIE! You salvaged 20 Coins.\n"; setColor(7);
            p1Acc->coins += 20;
        }
        saveAccount(p1Acc);
    }

    if (winnerName != "") { 
        string lbWinners[100];
        string lbLosers[100];
        int lbWins[100];
        int lbCount = 0;
        
        ifstream readLB("leaderboard.txt");
        while (readLB >> lbWinners[lbCount] >> lbLosers[lbCount] >> lbWins[lbCount] && lbCount < 100) {
            lbCount++;
        }
        readLB.close();
        
        bool found = false;
        for (int i = 0; i < lbCount; i++) {
            if (lbWinners[i] == winnerName && lbLosers[i] == loserName) {
                lbWins[i]++; 
                found = true;
                break;
            }
        }
        if (!found) { 
            lbWinners[lbCount] = winnerName;
            lbLosers[lbCount] = loserName;
            lbWins[lbCount] = 1;
            lbCount++;
        }
        
        ofstream writeLB("leaderboard.txt", ios::trunc); 
        for (int i = 0; i < lbCount; i++) writeLB << lbWinners[i] << " " << lbLosers[i] << " " << lbWins[i] << "\n";
        writeLB.close();
    }

    time_t now = time(0);
    char* dt = ctime(&now);
    
    ofstream logFile("match_logs.txt", ios::app); 
    logFile << "\n=== MATCH RECORDED: " << dt;
    logFile << "MATCHUP: " << names[0] << " VS " << names[1] << "\n";
    logFile << "WINNER: " << (winnerName != "" ? winnerName : "DRAW") << "\n";
    logFile << "Format: [Round] | [" << names[0] << " Dmg] | [" << names[1] << " Dmg] | [Hazard Event]\n";
    for (int i = 0; i < currentTurn; i++) {
        logFile << "Round " << i+1 << " | P1: " << battleLog[i][0] << " | P2: " << battleLog[i][1] 
                << " | Env: " << battleLog[i][2] << "\n";
    }
    logFile.close();
    cout << "  > Matchup and statistics successfully saved to database.\n";

    delete[] invP1;
    delete[] invP2;

    cout << "\n  Press any key to return to the Hub..."; 
    flushKeys(); _getch();
}

// ==========================================
// MAIN ENGINE (ANIMATED DRAW-IN)
// ==========================================
int main() {
    CREATE_DIR("accounts");
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
        
        flushKeys();
        char authChoice = _getch();

        if (authChoice == '4') {
            systemRunning = false;
            break;
        }

        if (authChoice == '1' || authChoice == '2') {
            cout << "\n\n  Enter Gladiator Name (No spaces): ";
            string nameInput;
            cin >> nameInput;
            cin.ignore(1000, '\n');
            cout << "  Enter Password: ";
            string passwordInput;
            cin >> passwordInput;
            cin.ignore(1000, '\n');
            
            if (authChoice == '1') {
                if (!loadAccount(nameInput, &currentAcc)) {
                    setColor(12); cout << "  > Account not found! Returning to menu...\n"; setColor(7);
                    this_thread::sleep_for(chrono::milliseconds(1500));
                    continue;
                }
                if (currentAcc.password.empty()) {
                    currentAcc.password = passwordInput;
                    saveAccount(&currentAcc);
                    setColor(14); cout << "  > Legacy account updated with a new password.\n"; setColor(7);
                } else if (currentAcc.password != passwordInput) {
                    setColor(12); cout << "  > Wrong password! Returning to menu...\n"; setColor(7);
                    this_thread::sleep_for(chrono::milliseconds(1500));
                    continue;
                }
                setColor(10); cout << "  > Login Successful! Welcome back, " << currentAcc.username << ".\n"; setColor(7);
            } else {
                if (loadAccount(nameInput, &currentAcc)) {
                    setColor(12); cout << "  > Account already exists! Returning to menu...\n"; setColor(7);
                    this_thread::sleep_for(chrono::milliseconds(1500));
                    continue;
                }
                currentAcc.username = nameInput;
                currentAcc.password = passwordInput;
                currentAcc.coins = 0;
                currentAcc.maxHp = 100;
                currentAcc.baseDamage = 15;
                currentAcc.overallLevel = 1;
                currentAcc.muscleLevel = 1;
                currentAcc.vitalityLevel = 1;
                saveAccount(&currentAcc);
                setColor(10); cout << "  > Account Created! Welcome to the Arena, " << currentAcc.username << ".\n"; setColor(7);
            }
            this_thread::sleep_for(chrono::milliseconds(1500));

            bool inHub = true;
            while (inHub) {
                clearScreen();
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

                flushKeys();
                char hubChoice = _getch();

                if (hubChoice == '1' || hubChoice == '2') {
                    int mode = (hubChoice == '1') ? 1 : 2;
                    PlayerAccount aiAcc = {"Mecha-Golem", "", 0, 150, 10 + (currentAcc.overallLevel * 2), 1, 1, 1};
                    string spMoves[2] = {"OMNISLASH", "OBLITERATION_BEAM"};
                    startBattle(&currentAcc, &aiAcc, false, spMoves, mode);
                } 
                else if (hubChoice == '3' || hubChoice == '4') {
                    int mode = (hubChoice == '3') ? 1 : 2;
                    cout << "\n\n  Enter Player 2 Name (Guest): ";
                    string p2Name;
                    cin >> p2Name;
                    PlayerAccount p2Acc = {p2Name, "", 0, 100, 15, 1, 1, 1};
                    string spMoves[2] = {"OMNISLASH", "METEOR_STRIKE"};
                    startBattle(&currentAcc, &p2Acc, true, spMoves, mode);
                }
                else if (hubChoice == '5') upgradeArea(&currentAcc);
                else if (hubChoice == '6') showLeaderboard();
                else if (hubChoice == '7') manageDataSystem();
                else if (hubChoice == '8') {
                    saveAccount(&currentAcc);
                    inHub = false;
                }
            }
        } else if (authChoice == '3') {
            cout << "\n\n  Enter Gladiator Name (No spaces): ";
            string nameInput;
            cin >> nameInput;
            cin.ignore(1000, '\n');
            cout << "  Enter Password: ";
            string passwordInput;
            cin >> passwordInput;
            cin.ignore(1000, '\n');

            if (!loadAccount(nameInput, &currentAcc)) {
                setColor(12); cout << "  > Account not found! Returning to menu...\n"; setColor(7);
                this_thread::sleep_for(chrono::milliseconds(1500));
                continue;
            }

            if (currentAcc.password.empty()) {
                currentAcc.password = passwordInput;
                saveAccount(&currentAcc);
                setColor(14); cout << "  > Legacy account updated with a new password.\n"; setColor(7);
            } else if (currentAcc.password != passwordInput) {
                setColor(12); cout << "  > Wrong password! Returning to menu...\n"; setColor(7);
                this_thread::sleep_for(chrono::milliseconds(1500));
                continue;
            }

            bool manageOpen = true;
            manageAccountSystem(&currentAcc, &manageOpen);
        }
    }

    clearScreen();
    cout << "\n  Shutting down system. Goodbye!\n";
    
#ifndef _WIN32
    cout << "\033[0m"; 
#endif
    return 0;
}