#include <swiftly/swiftly.h>
#include <swiftly/server.h>
#include <swiftly/database.h>
#include <swiftly/commands.h>
#include <swiftly/configuration.h>
#include <swiftly/logger.h>
#include <swiftly/timers.h>
#include <swiftly/gameevents.h>

Server *server = nullptr;
PlayerManager *g_playerManager = nullptr;
Database *db = nullptr;
Commands *commands = nullptr;
Configuration *config = nullptr;
Logger *logger = nullptr;
Timers *timers = nullptr;

void OnProgramLoad(const char *pluginName, const char *mainFilePath)
{
    Swiftly_Setup(pluginName, mainFilePath);

    server = new Server();
    g_playerManager = new PlayerManager();
    commands = new Commands(pluginName);
    config = new Configuration();
    logger = new Logger(mainFilePath, pluginName);
    timers = new Timers();
}

void OnPlayerSpawn(Player *player)
{
    if (!db->IsConnected())
        return;

    if (player->IsFirstSpawn() && !player->IsFakeClient())
        db->Query("insert ignore into `ranks` (steamid) values ('%llu')", player->GetSteamID());
}

void OnPluginStart()
{
        commands->Register("ranks", reinterpret_cast<void *>(&Command_Ranks));

        db = new Database("CONNECTION_NAME");

        DB_Result result = db->Query("CREATE TABLE IF NOT EXISTS `ranks` (`steamid` varchar(128) NOT NULL, `points` int(11) NOT NULL DEFAULT 0) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;");
        if (result.size() > 0)
            db->Query("ALTER TABLE `ranks` ADD UNIQUE KEY `steamid` (`steamid`);");
}

void Command_Ranks(int playerID, const char **args, uint32_t argsCount, bool silent)
{
     if (playerID == -1)
        return;
    if (!db->IsConnected())
        return;
        Player *player = g_playerManager->GetPlayer(playerID);
        if (player == nullptr)
        return;

    DB_Result result = db->Query("select * from `ranks` where steamid = '%llu' limit 1", player->GetSteamID());
    player->SendMsg(HUD_PRINTTALK, "{RED}[1TAP] {DEFAULT}Ai {LIGHTBLUE}%d {DEFAULT}puncte\n", (db->fetchValue<int>(result, 0, "points")));
}

void OnPlayerDeath(Player *player, Player *attacker, Player *assister, bool assistedflash, const char *weapon, bool headshot, short dominated, short revenge, short wipe, short penetrated, bool noreplay, bool noscope, bool thrusmoke, bool attackerblind, float distance, short dmg_health, short dmg_armor, short hitgroup)
{
    DB_Result result = db->Query("SELECT points FROM %s WHERE steamid = '%llu' LIMIT 1", "ranks", player->GetSteamID());
    int currentPoints = 0;
    if(result.size() > 0) {
        currentPoints = db->fetchValue<int>(result, 0, "points");
    }

    if(player == attacker) {
        if(currentPoints > 0) {
            player->SendMsg(HUD_PRINTTALK, "[1TAP] Your exp: - %d \n", currentPoints - 1);
            db->Query("UPDATE %s SET points = points - 2 WHERE steamid = '%llu' LIMIT 1", "ranks", player->GetSteamID());
        }
    }
    else if(headshot && attacker) {
        player->SendMsg(HUD_PRINTTALK, "[1TAP] Your exp: + %d \n", currentPoints + 10);
        db->Query("UPDATE %s SET points = points + 10 WHERE steamid = '%llu' LIMIT 1", "ranks", attacker->GetSteamID());
        if(currentPoints > 0) {
            player->SendMsg(HUD_PRINTTALK, "[1TAP] Your exp: - %d \n", currentPoints - 1);
            db->Query("UPDATE %s SET points = points - 1 WHERE steamid = '%llu' LIMIT 1", "ranks", player->GetSteamID());
        }
    }
    else if(noscope && attacker) {
        player->SendMsg(HUD_PRINTTALK, "[1TAP] Your exp: + %d (noscope)\n", currentPoints - 1);
        db->Query("UPDATE %s SET points = points + 10 WHERE steamid = '%llu' LIMIT 1", "ranks", attacker->GetSteamID());
        if(currentPoints > 0) {
            layer->SendMsg(HUD_PRINTTALK, "[1TAP] Your exp: - %d \n", currentPoints - 1);
            db->Query("UPDATE %s SET points = points - 2 WHERE steamid = '%llu' LIMIT 1", "ranks", player->GetSteamID());
        }
    }
    else if(attacker) {
    player->SendMsg(HUD_PRINTTALK, "[1TAP] Your exp: + %d \n", currentPoints - 1);
    db->Query("UPDATE %s SET points = points + 5 WHERE steamid = '%llu' LIMIT 1", "players_credits", attacker->GetSteamID());
    }
}

void OnPluginStop()
{
}

const char *GetPluginAuthor()
{
    return "blu1337";
}

const char *GetPluginVersion()
{
    return "1.0.0";
}

const char *GetPluginName()
{
    return "swiftly_rank";
}

const char *GetPluginWebsite()
{
    return "1tap.ro";
}