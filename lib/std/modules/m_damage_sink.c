/* Do not remove the headers from this file! see /USAGE for more info. */

// 04-98: Iizuka updated to conform to the new adversary code.
// 05-98: Iizuka added support for multiple resistances.

#include <armor.h>

inherit CLASS_EVENT_INFO;

private
mapping resistances = ([]);
private
mapping weaknesses = ([]);
private
int armor_class;

//: FUNCTION set_armor_class
// Set the protection of the particular damage sink.  random(class) points
// of damage will be prevented.
void set_armor_class(int x)
{
   armor_class = x;
   this_object()->set_max_durability(ARMOR_DURA_PER_AC * x);
}

//: FUNCTION query_armor_class
// Query the current armor class of an object.  Higher is better.
int query_armor_class()
{
   return armor_class;
}

mapping query_resistances()
{
   return resistances;
}

mapping query_weaknesses()
{
   return weaknesses;
}

class event_info sink_modify_event(class event_info evt)
{
   int reduced = evt.data[1];
   // TBUG(event_to_str(evt));
   if (stringp(evt.data))
      return evt;
   if (member_array(evt.data[0], keys(weaknesses)) != -1)
      evt.data[1] += weaknesses[evt.data[0]];
   if (member_array(evt.data[0], keys(resistances)) != -1)
      evt.data[1] -= resistances[evt.data[0]] + armor_class;
   else
      evt.data[1] -= ((armor_class / 2) + random(armor_class / 2));
   if (evt.data[1] < 0)
      evt.data[1] = 0;

   reduced -= evt.data[1];
   if (reduced && evt.weapon)
   {
      this_object()->decrease_durability(reduced);
      evt->weapon->decrease_durability(reduced);
   }
   // TBUG( "Outgoing dmg: "+evt.data[1]+" reduced: "+reduced);
   return evt;
}

//: FUNCTION set_resist
// set_resist(type, amt) specifies that for damage of type 'type', the armor
// class should be increased by 'amt'
void set_resist(string type, int amt)
{
   if (DAMAGE_D->query_valid_damage_type(type))
      resistances[type] = amt;
   else
      error(sprintf("Invalid damage type %s in %O\n", type, this_object()));
}

//: FUNCTION set_resistances
// void set_resistances(mapping) - Set all the resistances an armor
// possesses.
void set_resistances(mapping x)
{
   string *exclude = keys(x) - DAMAGE_D->query_damage_types();
   if (sizeof(exclude))
      error("Invalid damage type(s) : " + implode(exclude, ","));
   resistances = x;
}

//: FUNCTION set_weakness
// set_weakness(type, amt) specifies that for damage of type 'type', the armor
// class should be decreased by 'amt'
void set_weakness(string type, int amt)
{
   if (DAMAGE_D->query_valid_damage_type(type))
      weaknesses[type] = amt;
   else
      error(sprintf("Invalid damage type %s in %O\n", type, this_object()));
}

//: FUNCTION set_weaknesses
// void set_weaknesses(mapping) - Set all the weaknesses an armor
// possesses.
void set_weaknesses(mapping weak)
{
   string *exclude = keys(weak) - DAMAGE_D->query_damage_types();
   if (sizeof(exclude))
      error("Invalid damage type(s) : " + implode(exclude, ","));
   weaknesses = weak;
}

//: FUNCTION is_armor
// Returns 1 if the object is a damage_sink.
int is_armor()
{
   return 1;
}
