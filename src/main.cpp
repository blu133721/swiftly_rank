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

void Command_Ranks(int playerID, const char **args, uint32_t argsCount, bool silent)
{
     if (playerID == -1)
        return;
    if (!db->IsConnected())
        return;
        Player *player = g_playerManager->GetPlayer(playerID);
        if (player == nullptr)
        return;

    DB_Result result = db->Query("SELECT points FROM %s WHERE steamid = '%llu' LIMIT 1", "ranks", player->GetSteamID());
    int PointsCommand = 0;
        if(result.size() > 0) {
            PointsCommand = db->fetchValue<int>(result, 0, "points");
        }
    player->SendMsg(HUD_PRINTTALK, "{RED}[1TAP] {DEFAULT}Player {RED}%s {default} has {red} %d {default} credits", player->GetName(), PointsCommand);
}

void OnPluginStart()
{
        commands->Register("rank", reinterpret_cast<void *>(&Command_Ranks));

        db = new Database("CONNECTION_NAME");

        DB_Result result = db->Query("CREATE TABLE IF NOT EXISTS `ranks` (`steamid` varchar(128) NOT NULL, `points` int(11) NOT NULL DEFAULT 0) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;");
        if (result.size() > 0)
            db->Query("ALTER TABLE `ranks` ADD UNIQUE KEY `steamid` (`steamid`);");
}


void OnPlayerDeath(Player *player, Player *attacker, Player *assister, bool assistedflash, const char *weapon, bool headshot, short dominated, short revenge, short wipe, short penetrated, bool noreplay, bool noscope, bool thrusmoke, bool attackerblind, float distance, short dmg_health, short dmg_armor, short hitgroup)
{
    DB_Result resultPlayer = db->Query("SELECT points FROM %s WHERE steamid = '%llu' LIMIT 1", "ranks", player->GetSteamID());
    DB_Result resultAttacker;
    int currentPointsPlayer = 0;
    int currentPointsAttacker = 0;
    if(resultPlayer.size() > 0) {
        currentPointsPlayer = db->fetchValue<int>(resultPlayer, 0, "points");
    }

    if(attacker) {
        resultAttacker = db->Query("SELECT points FROM %s WHERE steamid = '%llu' LIMIT 1", "ranks", attacker->GetSteamID());
        if(resultAttacker.size() > 0) {
            currentPointsAttacker = db->fetchValue<int>(resultAttacker, 0, "points");
        }
    }

    if(player == attacker) {
        if(currentPointsPlayer > 0) {
             attacker->SendMsg(HUD_PRINTTALK, "{RED} [1TAP] {DEFAULT}Your exp: %d {RED}[-5 for suicide]\n", currentPointsPlayer - config->Fetch<int>("swiftly_ranks.Death"));
            db->Query("UPDATE %s SET points = points - %s WHERE steamid = '%llu' LIMIT 1", "ranks", player->GetSteamID(), config->Fetch<int>("swiftly_ranks.Death"));
        }
    }
    else if(headshot && attacker) {
         attacker->SendMsg(HUD_PRINTTALK, "{RED} [1TAP] {DEFAULT}Your exp: %d {RED}[+10 for headshot]\n", currentPointsAttacker + 10);
        db->Query("UPDATE %s SET points = points + 10 WHERE steamid = '%llu' LIMIT 1", "ranks", attacker->GetSteamID());
        if(currentPointsPlayer > 0) {
            player->SendMsg(HUD_PRINTTALK, "{RED} [1TAP] {DEFAULT}Your exp: %d {RED}[-2 for dying]\n", currentPointsPlayer - 2);
            db->Query("UPDATE %s SET points = points - 2 WHERE steamid = '%llu' LIMIT 1", "ranks", player->GetSteamID());
        }
    }
    else if(noscope && attacker) {
        attacker->SendMsg(HUD_PRINTTALK, "{RED} [1TAP] {DEFAULT}Your exp: %d {RED}[+10 for noscope]\n", currentPointsAttacker + 10);
        db->Query("UPDATE %s SET points = points + 10 WHERE steamid = '%llu' LIMIT 1", "ranks", attacker->GetSteamID());
        if(currentPointsPlayer > 0) {
            player->SendMsg(HUD_PRINTTALK, "{RED} [1TAP] {DEFAULT}Your exp: %d {RED}[-2 for dying]\n", currentPointsPlayer - 2);
            db->Query("UPDATE %s SET points = points - 2 WHERE steamid = '%llu' LIMIT 1", "ranks", player->GetSteamID());
        }
    }
    else if(attacker) {
        attacker->SendMsg(HUD_PRINTTALK, "{RED} [1TAP] {DEFAULT}Your exp: %d {RED}[+5 for kill]\n", currentPointsAttacker + config->Fetch<int>("swiftly_ranks.NormalKill"));
        db->Query("UPDATE %s SET points = points + %s WHERE steamid = '%llu' LIMIT 1", "ranks", attacker->GetSteamID(), config->Fetch<int>("swiftly_ranks.NormalKill"));
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