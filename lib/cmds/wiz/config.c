/* Do not remove the headers from this file! see /USAGE for more info. */

//: COMMAND
//$$ see: admtool
// USAGE: config <topic>
//        config help
//
// Shows the MUD configuration for different areas.

#include <stats.h>

#define HEADCOL "<036>"

inherit CMD;
inherit CLASS_STATMODS;

void config_races()
{
   mixed *races = get_dir("/std/race/*.c");
   int tick = 0;
   races = map(races, ( : load_object("/std/race/" + $1) :));

   printf(HEADCOL + "%20.20s %8s %8s %8s %8s %8s %8s %8s %8s", "Roll Mods", "STR Adj", "STR Rng", "AGI Adj",
          "AGI Rng", "INT Adj", "INT Rng", "WIL Adj", "WIL Rng");
   foreach (object ob in races)
   {
      class stat_roll_mods srm = ob->query_roll_mods();
      printf((tick % 2 == 0 ? "<015>" : "<117>") + "%20.20s %8d %8d %8d %8d %8d %8d %8d %8d ", ob->query_race(),
             srm.str_adjust, srm.str_range, srm.agi_adjust, srm.agi_range, srm.int_adjust, srm.int_range,
             srm.wil_adjust, srm.wil_range);
      tick++;
   }

   write("\n\n");
   printf(HEADCOL+"%20.20s %8s %8s %8s %8s %8s %8s %8s %8s", "Stat ranges", "STR", "AGI", "INT", "WIL", "CON+",
          "WIS+", "CHA+", "Adj sum");

   tick = 0;
   foreach (object ob in races)
   {
      class stat_roll_mods srm = ob->query_roll_mods();
      int con, wis, cha, adj_total;
      con = ob->racial_con_bonus();
      wis = ob->racial_wis_bonus();
      cha = ob->racial_cha_bonus();
      adj_total = srm.str_adjust + srm.agi_adjust + srm.int_adjust + srm.wil_adjust;

      printf((tick % 2 == 0 ? "<015>" : "<117>") + "%20.20s %8s %8s %8s %8s %8d %8d %8d " +
                 (adj_total != 0 ? "<009>" : "") + "%8s",
             ob->query_race(),
             (BASE_VALUE + srm.str_adjust - (srm.str_range / 2)) + "-" +
                 (BASE_VALUE + srm.str_adjust + (srm.str_range / 2)),
             (BASE_VALUE + srm.agi_adjust - (srm.agi_range / 2)) + "-" +
                 (BASE_VALUE + srm.agi_adjust + (srm.agi_range / 2)),
             (BASE_VALUE + srm.int_adjust - (srm.int_range / 2)) + "-" +
                 (BASE_VALUE + srm.int_adjust + (srm.int_range / 2)),
             (BASE_VALUE + srm.wil_adjust - (srm.wil_range / 2)) + "-" +
                 (BASE_VALUE + srm.wil_adjust + (srm.wil_range / 2)),
             con, wis, cha, ""+adj_total);
      tick++;
   }

   write("<res>\n\nStats are calculated as: " + BASE_VALUE + " + Adjust ± (Range/2)\n");
   write("VAL+ are bonuses that are just added to the derived stat.\n");
   write("'Adj sum' must equal 0.\n");
}

void config_topic(string topic)
{
   switch (topic)
   {
   case "races":
      config_races();
      break;
   default:
      write("Not supported yet.");
      break;
   }
}

private
void main(string *arg)
{
   if (arg[0] == "help")
      write("config: races (not much supported now)\n");
   else
      config_topic(arg[0]);
   return;
}
