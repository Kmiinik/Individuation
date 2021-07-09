#include "bn_log.h"
#include "bn_core.h"
#include "bn_math.h"
#include "bn_music.h"
#include "bn_random.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_bg_palettes.h"
#include "bn_music_items.h"
#include "bn_affine_bg_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_affine_bg_map_ptr.h"
#include "bn_bg_palette_actions.h"
#include "bn_affine_bg_attributes.h"
#include "bn_bg_palettes_actions.h"

#include "bn_sprite_items_needle.h"
#include "bn_sprite_items_compass.h"
#include "bn_sprite_items_portal_1.h"
#include "bn_sprite_items_portal_2.h"
#include "bn_sprite_items_portal_3.h"
#include "bn_affine_bg_items_tiles_tiles.h"
#include "bn_regular_bg_items_main_bg.h"
#include "bn_regular_bg_items_mirror_bg.h"
#include "bn_regular_bg_items_placeholder_title_screen.h"

namespace
{
	struct item
	{

	};

	struct creature
	{

		int x = -1;
		int y = -1;
		unsigned short int facing = 0;
		bool solid = true;

		//add stats and equipment, you can probably do equipment using a form of index, so like, the first "digit" is a 0, 1, 2, or 3 for if it is a weapon, armor, accessory, or consumable.
		//then the next could be either atk or prt depending on if it's armor or weapon, then basically just go down the rest of the stats, then one can be the trigger for the effect, then the cost, then the effect itself
		//idk if i'm actually going to do accessories, but consumables probably make sense, and you can basically just program it as if it was an item with limited charges that gets destroyed if it runs out, usually 1
		unsigned int current_health = 5;
		unsigned short int speed = 1;
		unsigned short int speed_timer = speed;
	};

	struct dungeonmap
	{
		static const int columns = 20;
		static const int rows = 20;
		static const int room_count = columns * rows;
		unsigned int random_number = 10;
		bn::random rng;

		creature creatures[room_count / 8];
		//0 should always be the player, 1 should always be the boss, the rest can be whatever. when not in use just set their position out of bounds

		unsigned short int rooms[room_count];
		//0 = no room, 1 = starting room, 2 = normal room, 3 = boss room, 4 = stairs, 5 = false wall
		//probably a better way to do this

		void randomize()
		{
			random_number = rng.get();
		}

		void generate_floor()
		{
			for (unsigned int i = 0; i < room_count; i++)
			{
				rooms[i] = 0;
			}
			creatures[0].x = 0;
			creatures[0].y = 0;
			random_number = rng.get();
			//setting starting room
			creatures[0].y = (random_number % (rows - 5)) + 3;
			creatures[0].x = (random_number % (columns - 2)) + 1;

			rooms[creatures[0].x + (creatures[0].y * columns)] = 1;
			rooms[creatures[0].x + ((creatures[0].y + 1) * columns)] = 2;

			int g_curser_x = creatures[0].x;
			int g_curser_y = creatures[0].y + 1;
			unsigned int direction; //0 = north, 1 = east, 2 = south, 3 = west
			random_number = rng.get();

			//setting hallways
			for (unsigned int i = 0; i < (random_number % (room_count / 4) + (room_count / 4)); i++)
			{
				random_number = rng.get();
				direction = random_number % 4;
				switch (direction)
				{
				case 0:
					if (g_curser_y < rows - 1)
					{
						if (rooms[g_curser_x + ((g_curser_y + 1) * columns)] == 0)
						{
							g_curser_y += 1;
						}
						else if (rooms[g_curser_x + ((g_curser_y + 2) * columns)] == 0 && g_curser_y < columns - 2)
						{
							g_curser_y += 2;
						}
					}
					break;
				case 1:
					if (g_curser_x < columns - 1)
					{
						if (rooms[(g_curser_x + 1) + (g_curser_y * columns)] == 0)
						{
							g_curser_x += 1;
						}
						else if (rooms[(g_curser_x + 2) + (g_curser_y * columns)] == 0 && g_curser_x < rows - 2)
						{
							g_curser_x += 2;
						}
					}
					break;
				case 2:
					if (g_curser_y > 0)
					{
						if (rooms[g_curser_x + ((g_curser_y - 1) * columns)] == 0)
						{
							g_curser_y -= 1;
						}
						else if (rooms[g_curser_x + ((g_curser_y - 2) * columns)] == 0 && g_curser_y > 1)
						{
							g_curser_y -= 2;
						}
					}
					break;
				case 3:
					if (g_curser_x > 0)
					{
						if (rooms[(g_curser_x - 1) + (g_curser_y * columns)] == 0)
						{
							g_curser_x -= 1;
						}
						else if (rooms[(g_curser_x - 2) + (g_curser_y * columns)] == 0 && g_curser_x > 1)
						{
							g_curser_x -= 2;
						}
					}
					break;
				default:
					BN_LOG("you messed something up, dumbass");
					break;
				}
				if (rooms[g_curser_x + (g_curser_y * columns)] != 1)
				{
					rooms[g_curser_x + (g_curser_y * columns)] = 2;
				}
			}
			if (rooms[g_curser_x + (g_curser_y * columns)] != 1)
			{
				rooms[g_curser_x + (g_curser_y * columns)] = 4;
			}
			else
			{
				rooms[creatures[0].x + ((creatures[0].y + 2) * columns)] = 4;
				BN_LOG("default");
			}

			//setting boss room
			unsigned short int potential_bossrooms = 0;
			short int potential_bossroom_x[columns];
			short int potential_bossroom_y[rows];

			unsigned short int less_preferred_bossrooms = 0;
			short int less_preferred_bossroom_x[columns];
			short int less_preferred_bossroom_y[rows];

			bool stranded[room_count];

			for (short int x = 1; x < columns - 1; x++)
			{
				for (short int y = 1; y < rows - 1; y++)
				{
					bool empty = true;
					bool stranded_checker = true;
					short int i = x + (y * columns);
					if (rooms[i - 1 + columns] != 0 ||
						rooms[i + columns] != 0 ||
						rooms[i + 1 + columns] != 0 ||
						rooms[i - 1] != 0 ||
						rooms[i] != 0 ||
						rooms[i + 1] != 0 ||
						rooms[i - 1 - columns] != 0 ||
						rooms[i - columns] != 0 ||
						rooms[i + 1 - columns] != 0)
					{
						empty = false;
					}
					else
					{
						less_preferred_bossroom_x[less_preferred_bossrooms] = x;
						less_preferred_bossroom_y[less_preferred_bossrooms] = y;
						less_preferred_bossrooms++;
						BN_LOG("space found");

						short int connected_rooms_north = 0;
						short int connected_rooms_east = 0;
						short int connected_rooms_south = 0;
						short int connected_rooms_west = 0;
						for (int j = -1; j < 2; j++)
						{
							if (y < rows - 2)
							{
								if (rooms[((x + j) + ((y + 2) * columns))] != 0)
								{
									connected_rooms_north++;
									stranded_checker = false;
								}
								if (connected_rooms_north > 1)
								{
									empty = false;
								}
							}
							if (x < columns - 2)
							{
								if (rooms[((x + 2) + ((y + j) * columns))] != 0)
								{
									connected_rooms_east++;
									stranded_checker = false;
								}
								if (connected_rooms_east > 1)
								{
									empty = false;
								}
							}
							if (y > 1)
							{
								if (rooms[(x + j) + ((y - 2) * columns)] != 0)
								{
									connected_rooms_south++;
									stranded_checker = false;
								}
								if (connected_rooms_south > 1)
								{
									empty = false;
								}
							}
							if (x > 1)
							{
								if (rooms[((x - 2) + ((y + j) * columns))] != 0)
								{
									connected_rooms_west++;
									stranded_checker = false;
								}
								if (connected_rooms_west > 1)
								{
									empty = false;
								}
							}
						}
					}
					if (empty)
					{
						stranded[potential_bossrooms] = stranded_checker;
						potential_bossroom_x[potential_bossrooms] = x;
						potential_bossroom_y[potential_bossrooms] = y;
						potential_bossrooms++;
						BN_LOG("good space found");
					}
				}
			}
			short int bossroom_x;
			short int bossroom_y;
			bool actually_stranded = false;
			if (potential_bossrooms > 0)
			{
				unsigned short int choice = (random_number / 100) % potential_bossrooms;
				rooms[potential_bossroom_x[choice] + (potential_bossroom_y[choice] * columns)] = 3;
				bossroom_x = potential_bossroom_x[choice];
				bossroom_y = potential_bossroom_y[choice];
				actually_stranded = stranded[choice];
			}
			else if (less_preferred_bossrooms > 0)
			{
				unsigned short int choice = (random_number / 100) % less_preferred_bossrooms;
				rooms[less_preferred_bossroom_x[choice] + (less_preferred_bossroom_y[choice] * columns)] = 3;
				bossroom_x = less_preferred_bossroom_x[choice];
				bossroom_y = less_preferred_bossroom_y[choice];
			}
			else
			{
				rooms[creatures[0].x + ((creatures[0].y - 2) * columns)] = 3;
				bossroom_x = creatures[0].x;
				bossroom_y = creatures[0].y - 2;
				BN_LOG("default placement");
			}
			for (int x = -1; x < 2; x++)
			{
				for (int y = -1; y < 2; y++)
				{
					if (rooms[((bossroom_x + x) + ((bossroom_y + y) * columns))] == 0)
					{
						rooms[((bossroom_x + x) + ((bossroom_y + y) * columns))] = 2;
					}
				}
			}

			creatures[1].x = bossroom_x;
			creatures[1].y = bossroom_y;

			if (actually_stranded)
			{
				BN_LOG("where are you?");
				bool room_found = false;
				unsigned short int movetowards = 0;
				int x = bossroom_x;
				int y = bossroom_y;

				for (int i = 2; i < (columns + rows) / 2; i++)
				{
					//north
					if (bossroom_y + i < rows && rooms[(bossroom_x)+((bossroom_y + i) * columns)] != 0 && !room_found)
					{
						x = bossroom_x;
						y = bossroom_y + i;
						room_found = true;
						movetowards = 0;
					}
					//east
					if (bossroom_x + i < columns && rooms[(bossroom_x + i) + ((bossroom_y)*columns)] != 0 && !room_found)
					{
						x = bossroom_x + i;
						y = bossroom_y;
						room_found = true;
						movetowards = 1;
					}
					//south
					if (bossroom_y - i < rows && rooms[(bossroom_x)+((bossroom_y - i) * columns)] != 0 && !room_found)
					{
						x = bossroom_x;
						y = bossroom_y - i;
						room_found = true;
						movetowards = 2;
					}
					//west
					if (bossroom_x - i < columns && rooms[(bossroom_x - i) + ((bossroom_y)*columns)] != 0 && !room_found)
					{
						x = bossroom_x - i;
						y = bossroom_y;
						room_found = true;
						movetowards = 3;
					}
					//northeast
					if (bossroom_y + i < rows && bossroom_x + i < columns && rooms[(bossroom_x + i) + ((bossroom_y + i) * columns)] != 0 && !room_found)
					{
						x = bossroom_x + i;
						y = bossroom_y + i;
						room_found = true;
						movetowards = 4;
					}
					//southeast
					if (bossroom_y - i < rows && bossroom_x + i < columns && rooms[(bossroom_x + i) + ((bossroom_y - i) * columns)] != 0 && !room_found)
					{
						x = bossroom_x + i;
						y = bossroom_y - i;
						room_found = true;
						movetowards = 5;
					}
					//southwest
					if (bossroom_y - i < rows && bossroom_x - i < columns && rooms[(bossroom_x - i) + ((bossroom_y - i) * columns)] != 0 && !room_found)
					{
						x = bossroom_x - i;
						y = bossroom_y - i;
						room_found = true;
						movetowards = 6;
					}
					//northwest
					if (bossroom_y + i < rows && bossroom_x - i < columns && rooms[(bossroom_x - i) + ((bossroom_y + i) * columns)] != 0 && !room_found)
					{
						x = bossroom_x - i;
						y = bossroom_y + i;
						room_found = true;
						movetowards = 7;
					}
					if (room_found)
					{
						BN_LOG(movetowards);
						break;
					}
				}
				if (room_found)
				{
					BN_LOG("found you!");
					short int b_curser_x = bossroom_x;
					short int b_curser_y = bossroom_y;
					switch (movetowards)
					{
					case 0:
						b_curser_y += 2;
						while (b_curser_y != y)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_y++;
						}
						break;

					case 1:
						b_curser_x += 2;
						while (b_curser_x != x)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_x++;
						}
						break;
					case 2:
						b_curser_y -= 2;
						while (b_curser_y != y)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_y--;
						}
						break;
					case 3:
						b_curser_x -= 2;
						while (b_curser_x != x)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_x--;
						}
						break;
					case 4:
						b_curser_y += 2;
						while (b_curser_y != y)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_y++;
						}
						while (b_curser_x != x)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_x++;
						}
						break;
					case 5:
						b_curser_y -= 2;
						while (b_curser_y != y)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_y--;
						}
						while (b_curser_x != x)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_x++;
						}
						break;
					case 6:
						b_curser_y -= 2;
						while (b_curser_y != y)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_y--;
						}
						while (b_curser_x != x)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_x--;
						}
						break;
					case 7:
						b_curser_y += 2;
						while (b_curser_y != y)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_y++;
						}
						while (b_curser_x != x)
						{
							if (rooms[b_curser_x + (b_curser_y * columns)] == 0)
							{
								rooms[b_curser_x + (b_curser_y * columns)] = 2;
							}
							else
							{
								break;
							}
							b_curser_x--;
						}
						break;
					default:
						break;
					}
				}
				else
				{
					BN_LOG("panic");
					generate_floor();
				}
			}

			//setting encounters (enemies and chests)
			random_number = rng.get();

			BN_LOG("e");
			for (int i = 0; i < room_count; i++)
			{
				BN_LOG(rooms[i]);
			}
		}

		void move(unsigned short int target_index, bool forwards)
		{
			creature target = creatures[target_index];
			//add detection of other creatures, i'm too tired to do that right now

			BN_LOG(creatures[target_index].x);
			BN_LOG(creatures[target_index].y);

			switch (target.facing)
			{
			case 0:
				if (forwards && target.y < rows - 1 && rooms[(target.x) + ((target.y + 1) * columns)] != 0)
				{
					target.y += 1;
				}
				else if (!forwards && target.y > 0 && rooms[(target.x) + ((target.y - 1) * columns)] != 0)
				{
					target.y -= 1;
				}
				break;
			case 1:
				if (forwards && target.x < columns - 1 && rooms[(target.x + 1) + ((target.y) * columns)] != 0)
				{
					target.x += 1;
				}
				else if (!forwards && target.x > 0 && rooms[(target.x - 1) + ((target.y) * columns)] != 0)
				{
					target.x -= 1;
				}
				break;
			case 2:
				if (!forwards && target.y < rows - 1 && rooms[(target.x) + ((target.y + 1) * columns)] != 0)
				{
					target.y += 1;
				}
				else if (forwards && target.y > 0 && rooms[(target.x) + ((target.y - 1) * columns)] != 0)
				{
					target.y -= 1;
				}
				break;
			case 3:
				if (!forwards && target.x < columns - 1 && rooms[(target.x + 1) + ((target.y) * columns)] != 0)
				{
					target.x += 1;
				}
				else if (forwards && target.x > 0 && rooms[(target.x - 1) + ((target.y) * columns)] != 0)
				{
					target.x -= 1;
				}
				break;
			default:
				break;
			}
			creatures[target_index].x = target.x;
			creatures[target_index].y = target.y;
			BN_LOG(creatures[target_index].x);
			BN_LOG(creatures[target_index].y);

		}

		void turn(unsigned short int target_index, bool right)
		{
			creature target = creatures[target_index];
			if (right)
			{
				if (target.facing < 3)
				{
					target.facing += 1;
				}
				else
				{
					target.facing = 0;
				}
			}
			else
			{
				if (target.facing > 0)
				{
					target.facing -= 1;
				}
				else
				{
					target.facing = 3;
				}
			}
			creatures[target_index].facing = target.facing;
		}

		bn::sprite_ptr render_portal(bn::sprite_ptr target, bool unmirrored) 
		{
			target.set_visible(false);
			int relative_x = 0;
			int relative_y = 0;
			creature player = creatures[0];

			switch (player.facing)
			{
			case 0:
				for (int y = 3; y > 0; y--)
				{
					for (int x = -2; x < 2; x++)
					{
						if (rooms[(x + player.x) + ((y + player.y) * columns)] == 4 && x + player.x >= 0 && x + player.x < columns && y + player.y < rows)
						{
							relative_x = x;
							relative_y = y;
						}
					}
				}
				break;
			case 1:
				for (int x = 3; x > 0; x--)
				{
					for (int y = 1; y >= -1; y--)
					{
						if (rooms[(x + player.x) + ((y + player.y) * columns)] == 4 && y + player.y >= 0 && y + player.y < rows && x + player.x < columns)
						{
							relative_x = y * -1;
							relative_y = x;
						}
					}
				}
				break;
			case 2:
				for (int y = 3; y > 0; y--)
				{
					for (int x = -2; x < 2; x++)
					{
						if (rooms[(player.x - x) + ((player.y - y) * columns)] == 4 && x + player.x >= 0 && x + player.x < columns && y + player.y >= 0)
						{
							relative_x = x;
							relative_y = y;
						}
					}
				}
				break;
			case 3:
				for (int x = 3; x > 0; x--)
				{
					for (int y = 1; y >= -1; y--)
					{
						if (rooms[(player.x - x) + ((player.y - y) * columns)] == 4 && y + player.y >= 0 && y + player.y < rows && x + player.x >= 0)
						{
							relative_x = y * -1;
							relative_y = x;
						}
					}
				}
				break;
			default:
				break;
			}
			if (unmirrored)
			{
				switch (relative_y)
				{
				case 1:
					target = bn::sprite_items::portal_1.create_sprite(-32, 30);
					switch (relative_x)
					{
					case -1:
						target.set_x(target.x() - 64);
						break;
					case 0:
						break;
					case 1:
						target.set_x(target.x() + 64);
						break;
					default:
						target.set_visible(false);
						break;
					}
					break;
				case 2:
					target = bn::sprite_items::portal_2.create_sprite(-16, 12);
					switch (relative_x)
					{
					case -1:
						target.set_x(target.x() - 32);
						break;
					case 0:
						break;
					case 1:
						target.set_x(target.x() + 32);
						break;
					default:
						target.set_visible(false);
						break;
					}
					break;
				case 3:
					target = bn::sprite_items::portal_3.create_sprite(-8, 0);
					switch (relative_x)
					{
					case -1:
						target.set_x(target.x() - 16);
						break;
					case 0:
						break;
					case 1:
						target.set_x(target.x() + 16);
						break;
					default:
						target.set_visible(false);
						break;
					}
					break;
				default:
					break;
				}
			}
			else
			{
				switch (relative_y)
				{
				case 1:
					target = bn::sprite_items::portal_1.create_sprite(32, 30);
					target.set_horizontal_flip(true);
					switch (relative_x)
					{
					case -1:
						target.set_x(target.x() - 64);
						break;
					case 0:
						break;
					case 1:
						target.set_x(target.x() + 64);
						break;
					default:
						target.set_visible(false);
						break;
					}
					break;
				case 2:
					target = bn::sprite_items::portal_2.create_sprite(16, 12);
					target.set_horizontal_flip(true);
					switch (relative_x)
					{
					case -1:
						target.set_x(target.x() - 32);
						break;
					case 0:
						break;
					case 1:
						target.set_x(target.x() + 32);
						break;
					default:
						target.set_visible(false);
						break;
					}
					break;
				case 3:
					target = bn::sprite_items::portal_3.create_sprite(8, 0);
					target.set_horizontal_flip(true);
					switch (relative_x)
					{
					case -1:
						target.set_x(target.x() - 16);
						break;
					case 0:
						break;
					case 1:
						target.set_x(target.x() + 16);
						break;
					default:
						target.set_visible(false);
						break;
					}
					break;
				default:
					break;
				}
			}
			return target;
		}
	};

	struct bg_map
	{
		static const int columns = 32;
		static const int rows = 32;
		static const int cells_count = columns * rows;


		bn::affine_bg_map_cell cells[cells_count];

		bg_map()
		{
			reset();
		}

		void render(dungeonmap floor)
		{
			creature player = floor.creatures[0];
			BN_LOG(player.x);
			BN_LOG(player.y);

			unsigned short int rooms[floor.room_count];
			short int vision[17];
			short int v = 0; //short for vision iterator

			for (int i = 0; i < floor.room_count; i++)
			{
				rooms[i] = floor.rooms[i];
			}
			for (int i = 0; i < 17; i++)
			{
				vision[i] = 0;
			}

			switch (player.facing)
			{
			case 0:
				for (int y = player.y + 3; y > player.y; y--)
				{
					for (int x = player.x - 2; x < player.x + 3; x++)
					{
						if (x >= floor.columns || x < 0 || y >= floor.rows || y < 0)
						{
							vision[v] = 0;
							BN_LOG("sports");
						}
						else
						{
							vision[v] = rooms[x + (y * floor.columns)];
						}
						v++;
					}
				}
				
				if (player.x - 1 >= 0)
				{
					vision[15] = rooms[(player.x - 1) + (player.y * floor.columns)];
				}
				if (player.x + 1 < floor.columns)
				{
					vision[16] = rooms[(player.x + 1) + (player.y * floor.columns)];
				}
				break;
			case 1:
				for (int x = player.x + 3; x > player.x; x--)
				{
					for (int y = player.y + 2; y > player.y - 3; y--)
					{
						if (x >= floor.columns || x < 0|| y >= floor.rows || y < 0)
						{
							vision[v] = 0;
							BN_LOG("sports");
						}
						else
						{
							vision[v] = rooms[x + (y * floor.columns)];
						}
						v++;
					}
				}

				if (player.y + 1 < floor.rows)
				{
					vision[15] = rooms[player.x + ((player.y + 1) * floor.columns)];
				}
				if (player.y - 1 >= 0)
				{
					vision[16] = rooms[player.x + ((player.y - 1) * floor.columns)];
				}
				break;
			case 2:
				for (int y = player.y - 3; y < player.y; y++)
				{
					for (int x = player.x + 2; x > player.x - 3; x--)
					{
						if (x >= floor.columns || x < 0|| y >= floor.rows || y < 0)
						{
							vision[v] = 0;
							BN_LOG("sports");
						}
						else
						{
							vision[v] = rooms[x + (y * floor.columns)];
						}
						v++;
					}
				}

				if (player.x + 1 < floor.columns)
				{
					vision[15] = rooms[(player.x + 1) + (player.y * floor.columns)];
				}
				if (player.x - 1 >= 0)
				{
					vision[16] = rooms[(player.x - 1) + (player.y * floor.columns)];
				}
				break;
			case 3:
				for (int x = player.x - 3; x < player.x; x++)
				{
					for (int y = player.y - 2; y < player.y + 3; y++)
					{
						if (x >= floor.columns || x < 0 || y >= floor.rows || y < 0)
						{
							vision[v] = 0;
							BN_LOG("sports");
						}
						else
						{
							vision[v] = rooms[x + (y * floor.columns)];
						}
						v++;
					}
				}

				if (player.y - 1 >= 0)
				{
					vision[15] = rooms[player.x + ((player.y - 1) * floor.columns)];
				}
				if (player.y + 1 < floor.rows)
				{
					vision[16] = rooms[player.x + ((player.y + 1) * floor.columns)];
				}
				break;
			default:
				break;
			}

			if (vision[0] == 0 || vision[0] == 5)
			{
				far_left_wall_3();
			}
			if (vision[4] == 0 || vision[4] == 5)
			{
				far_right_wall_3();
			}
			if (vision[1] == 0 || vision[1] == 5)
			{
				perpendicular_left_wall_3();
			}
			if (vision[3] == 0 || vision[3] == 5)
			{
				perpendicular_right_wall_3();
			}
			if (vision[2] == 0 || vision[2] == 5)
			{
				perpendicular_forward_wall_3();
			}

			if (vision[5] == 0 || vision[5] == 5)
			{
				far_left_wall_2();
			}
			if (vision[9] == 0 || vision[9] == 5)
			{
				far_right_wall_2();
			}
			if (vision[6] == 0 || vision[6] == 5)
			{
				left_wall_2();
			}
			if (vision[8] == 0 || vision[8] == 5)
			{
				right_wall_2();
			}
			if (vision[7] == 0 || vision[7] == 5)
			{
				perpendicular_forward_wall_2();
			}

			if (vision[10] == 0 || vision[10] == 5)
			{
				far_left_wall_1();
			}
			if (vision[14] == 0 || vision[14] == 5)
			{
				far_right_wall_1();
			}
			if (vision[11] == 0 || vision[11] == 5)
			{
				left_wall_1();
			}
			if (vision[13] == 0 || vision[13] == 5)
			{
				right_wall_1();
			}
			if (vision[12] == 0 || vision[12] == 5)
			{
				perpendicular_forward_wall_1();
			}

			if (vision[15] == 0 || vision[15] == 5)
			{
				left_wall_0();
			}
			if (vision[16] == 0 || vision[16] == 5)
			{
				right_wall_0();
			}
		}


		//you can get rid of most of the perpindicular wall functions by making the normal walls also draw a perpendicular wall, because the perpendicular wall is just another wall
		//of the "room" you see from the normal wall, so there will never just be a flat wall
		void left_wall_0()
		{
			cells[193] = 1;
			cells[194] = 1;
			cells[195] = 1;
			cells[196] = 16;
			cells[197] = 17;

			cells[225] = 1;
			cells[226] = 1;
			cells[227] = 1;
			cells[228] = 1;
			cells[229] = 1;
			cells[230] = 16;

			for (int iy = 8; iy < 19; iy++)
			{
				for (int ix = 1; ix < 7; ix++)
				{
					cells[ix + (iy * 32)] = 1;
				}
			}

			cells[609] = 1;
			cells[610] = 1;
			cells[611] = 1;
			cells[612] = 1;
			cells[613] = 1;
			cells[614] = 34;

			cells[641] = 1;
			cells[642] = 1;
			cells[643] = 1;
			cells[644] = 34;
			cells[645] = 35;

			cells[673] = 1;
			cells[674] = 34;
			cells[675] = 35;

			cells[705] = 35;
		}
		void left_wall_1()
		{
			cells[7 + (7 * columns)] = 21;

			cells[7 + (8 * columns)] = 2;
			cells[8 + (8 * columns)] = 20;
			cells[9 + (8 * columns)] = 21;

			cells[7 + (9 * columns)] = 2;
			cells[8 + (9 * columns)] = 2;
			cells[9 + (9 * columns)] = 2;
			cells[10 + (9 * columns)] = 20;

			for (int y = 10; y < 17; y++)
			{
				for (int x = 7; x < 11; x++)
				{
					cells[x + (y * columns)] = 2;
				}
			}
			for (int y = 7; y < 20; y++)
			{
				for (int x = 1; x < 7; x++)
				{
					if (y == 7)
					{
						cells[x + (y * columns)] = 6;
					}
					else if (y == 19)
					{
						cells[x + (y * columns)] = 7;
					}
					else
					{
						cells[x + (y * columns)] = 1;
					}
				}
			}

			cells[7 + (17 * columns)] = 2;
			cells[8 + (17 * columns)] = 2;
			cells[9 + (17 * columns)] = 2;
			cells[10 + (17 * columns)] = 38;

			cells[7 + (18 * columns)] = 2;
			cells[8 + (18 * columns)] = 38;
			cells[9 + (18 * columns)] = 39;

			cells[7 + (19 * columns)] = 39;
		}
		void left_wall_2()
		{
			cells[11 + (9 * columns)] = 25;

			cells[11 + (10 * columns)] = 3;
			cells[12 + (10 * columns)] = 24;
			cells[13 + (10 * columns)] = 25;

			for (int y = 11; y < 16; y++)
			{
				for (int x = 11; x < 14; x++)
				{
					cells[x + (y * columns)] = 3;
				}
			}
			for (int y = 9; y < 18; y++)
			{
				for (int x = 4; x < 11; x++)
				{
					if (y == 9)
					{
						cells[x + (y * columns)] = 8;
					}
					else if (y == 17)
					{
						cells[x + (y * columns)] = 9;
					}
					else
					{
						cells[x + (y * columns)] = 2;
					}
				}
			}

			cells[11 + (16 * columns)] = 3;
			cells[12 + (16 * columns)] = 42;
			cells[13 + (16 * columns)] = 43;

			cells[11 + (17 * columns)] = 43;
		}
		void far_left_wall_1()
		{
			cells[1 + (8 * columns)] = 68;

			cells[3 + (9 * columns)] = 67;
			cells[2 + (9 * columns)] = 66;
			cells[1 + (9 * columns)] = 3;

			for (int y = 10; y < 17; y++)
			{
				for (int x = 1; x < 4; x++)
				{
					cells[x + (y * columns)] = 3;
				}
			}

			cells[3 + (17 * columns)] = 52;
			cells[2 + (17 * columns)] = 51;
			cells[1 + (17 * columns)] = 3;

			cells[1 + (18 * columns)] = 53;
		}
		void far_left_wall_2()
		{
			cells[4 + (9 * columns)] = 74;

			cells[4 + (10 * columns)] = 4;
			cells[5 + (10 * columns)] = 72;
			cells[6 + (10 * columns)] = 73;
			cells[7 + (10 * columns)] = 74;

			for (int y = 11; y < 16; y++)
			{
				for (int x = 4; x < 8; x++)
				{
					cells[x + (y * columns)] = 4;
				}
			}

			for (int y = 9; y < 18; y++)
			{
				for (int x = 1; x < 4; x++)
				{
					if (y == 9)
					{
						cells[x + (y * columns)] = 10;
					}
					else if (y == 17)
					{
						cells[x + (y * columns)] = 11;
					}
					else
					{
						cells[x + (y * columns)] = 3;
					}
				}
			}

			cells[4 + (16 * columns)] = 4;
			cells[5 + (16 * columns)] = 57;
			cells[6 + (16 * columns)] = 58;
			cells[7 + (16 * columns)] = 59;

			cells[4 + (17 * columns)] = 59;
		}
		void far_left_wall_3()
		{
			cells[8 + (11 * columns)] = 78;
			cells[9 + (11 * columns)] = 79;

			for (int y = 12; y < 15; y++)
			{
				for (int x = 8; x < 10; x++)
				{
					cells[x + (y * columns)] = 5;
				}
			}
			for (int y = 11; y < 16; y++)
			{
				for (int x = 4; x < 8; x++)
				{
					cells[x + (y * columns)] = 4;
				}
			}


			cells[8 + (15 * columns)] = 63;
			cells[9 + (15 * columns)] = 64;
		}
		void perpendicular_forward_wall_1()
		{
			for (int y = 7; y < 20; y++)
			{
				for (int x = 7; x < 25; x++)
				{
					if (y == 7)
					{
						cells[x + (y * columns)] = 8;
					}
					else if (y == 19)
					{
						cells[x + (y * columns)] = 9;
					}
					else
					{
						cells[x + (y * columns)] = 2;
					}
				}
			}
		}
		void perpendicular_forward_wall_2()
		{
			for (int y = 9; y < 18; y++)
			{
				for (int x = 11; x < 21; x++)
				{
					if (y == 9)
					{
						cells[x + (y * columns)] = 10;
					}
					else if (y == 17)
					{
						cells[x + (y * columns)] = 11;
					}
					else
					{
						cells[x + (y * columns)] = 3;
					}
				}
			}
		}
		void perpendicular_left_wall_3()
		{
			cells[14 + (11 * columns)] = 28;

			for (int y = 12; y < 15; y++)
			{
				cells[14 + (y * columns)] = 4;
			}

			cells[14 + (15 * columns)] = 46;
			for (int y = 11; y < 16; y++)
			{
				for (int x = 8; x < 14; x++)
				{
					cells[x + (y * columns)] = 4;
				}
			}
		}
		void perpendicular_forward_wall_3()
		{
			for (int y = 11; y < 16; y++)
			{
				for (int x = 14; x < 18; x++)
				{
					cells[x + (y * columns)] = 4;
				}
			}
		}
		void perpendicular_right_wall_3()
		{
			cells[17 + (11 * columns)] = 31;

			for (int y = 12; y < 15; y++)
			{
				cells[17 + (y * columns)] = 4;
			}

			cells[17 + (15 * columns)] = 45;

			for (int y = 11; y < 16; y++)
			{
				for (int x = 18; x < 24; x++)
				{
					cells[x + (y * columns)] = 4;
				}
			}
		}
		void far_right_wall_1()
		{
			cells[30 + (8 * columns)] = 69;

			cells[28 + (9 * columns)] = 70;
			cells[29 + (9 * columns)] = 71;
			cells[30 + (9 * columns)] = 3;

			for (int y = 10; y < 17; y++)
			{
				for (int x = 28; x < 31; x++)
				{
					cells[x + (y * columns)] = 3;
				}
			}

			cells[28 + (17 * columns)] = 49;
			cells[29 + (17 * columns)] = 50;
			cells[30 + (17 * columns)] = 3;

			cells[30 + (18 * columns)] = 48;
		}
		void far_right_wall_2()
		{
			cells[27 + (9 * columns)] = 75;

			cells[24 + (10 * columns)] = 75;
			cells[25 + (10 * columns)] = 76;
			cells[26 + (10 * columns)] = 77;
			cells[27 + (10 * columns)] = 4;

			for (int y = 11; y < 16; y++)
			{
				for (int x = 24; x < 28; x++)
				{
					cells[x + (y * columns)] = 4;
				}
			}

			for (int y = 9; y < 18; y++)
			{
				for (int x = 28; x < 31; x++)
				{
					if (y == 9)
					{
						cells[x + (y * columns)] = 10;
					}
					else if (y == 17)
					{
						cells[x + (y * columns)] = 11;
					}
					else
					{
						cells[x + (y * columns)] = 3;
					}
				}
			}

			cells[24 + (16 * columns)] = 54;
			cells[25 + (16 * columns)] = 55;
			cells[26 + (16 * columns)] = 56;
			cells[27 + (16 * columns)] = 4;

			cells[27 + (17 * columns)] = 54;
		}
		void far_right_wall_3()
		{
			cells[22 + (11 * columns)] = 82;
			cells[23 + (11 * columns)] = 83;

			for (int y = 12; y < 15; y++)
			{
				for (int x = 22; x < 24; x++)
				{
					cells[x + (y * columns)] = 5;
				}
			}
			for (int y = 11; y < 16; y++)
			{
				for (int x = 24; x < 28; x++)
				{
					cells[x + (y * columns)] = 4;
				}
			}


			cells[22 + (15 * columns)] = 61;
			cells[23 + (15 * columns)] = 62;
		}
		void right_wall_0()
		{
			cells[26 + (6 * columns)] = 18;
			cells[27 + (6 * columns)] = 19;
			cells[28 + (6 * columns)] = 1;
			cells[29 + (6 * columns)] = 1;
			cells[30 + (6 * columns)] = 1;

			cells[25 + (7 * columns)] = 19;
			cells[26 + (7 * columns)] = 1;
			cells[27 + (7 * columns)] = 1;
			cells[28 + (7 * columns)] = 1;
			cells[29 + (7 * columns)] = 1;
			cells[30 + (7 * columns)] = 1;

			for (int y = 8; y < 19; y++)
			{
				for (int x = 25; x < 31; x++)
				{
					cells[x + (y * columns)] = 1;
				}
			}

			cells[25 + (19 * columns)] = 33;
			cells[26 + (19 * columns)] = 1;
			cells[27 + (19 * columns)] = 1;
			cells[28 + (19 * columns)] = 1;
			cells[29 + (19 * columns)] = 1;
			cells[30 + (19 * columns)] = 1;

			cells[26 + (20 * columns)] = 32;
			cells[27 + (20 * columns)] = 33;
			cells[28 + (20 * columns)] = 1;
			cells[29 + (20 * columns)] = 1;
			cells[30 + (20 * columns)] = 1;

			cells[28 + (21 * columns)] = 32;
			cells[29 + (21 * columns)] = 33;
			cells[30 + (21 * columns)] = 1;

			cells[30 + (22 * columns)] = 32;
		}
		void right_wall_1()
		{
			cells[24 + (7 * columns)] = 22;

			cells[22 + (8 * columns)] = 22;
			cells[23 + (8 * columns)] = 23;
			cells[24 + (8 * columns)] = 2;

			cells[21 + (9 * columns)] = 23;
			cells[22 + (9 * columns)] = 2;
			cells[23 + (9 * columns)] = 2;
			cells[24 + (9 * columns)] = 2;

			for (int y = 10; y < 17; y++)
			{
				for (int x = 21; x < 25; x++)
				{
					cells[x + (y * columns)] = 2;
				}
			}
			for (int y = 7; y < 20; y++)
			{
				for (int x = 25; x < 31; x++)
				{
					if (y == 7)
					{
						cells[x + (y * columns)] = 6;
					}
					else if (y == 19)
					{
						cells[x + (y * columns)] = 7;
					}
					else
					{
						cells[x + (y * columns)] = 1;
					}
				}
			}

			cells[21 + (17 * columns)] = 37;
			cells[22 + (17 * columns)] = 2;
			cells[23 + (17 * columns)] = 2;
			cells[24 + (17 * columns)] = 2;


			cells[22 + (18 * columns)] = 36;
			cells[23 + (18 * columns)] = 37;
			cells[24 + (18 * columns)] = 2;

			cells[24 + (19 * columns)] = 36;
		}
		void right_wall_2()
		{
			cells[20 + (9 * columns)] = 26;

			cells[18 + (10 * columns)] = 26;
			cells[19 + (10 * columns)] = 27;
			cells[20 + (10 * columns)] = 3;

			for (int y = 11; y < 16; y++)
			{
				for (int x = 18; x < 21; x++)
				{
					cells[x + (y * columns)] = 3;
				}
			}
			for (int y = 9; y < 18; y++)
			{
				for (int x = 21; x < 28; x++)
				{
					if (y == 9)
					{
						cells[x + (y * columns)] = 8;
					}
					else if (y == 17)
					{
						cells[x + (y * columns)] = 9;
					}
					else
					{
						cells[x + (y * columns)] = 2;
					}
				}
			}

			cells[18 + (16 * columns)] = 40;
			cells[19 + (16 * columns)] = 41;
			cells[20 + (16 * columns)] = 3;

			cells[20 + (17 * columns)] = 40;
		}

		void reset()
		{
			bn::memory::clear(cells_count, cells[0]);
		}
	};

	dungeonmap titlescreen(dungeonmap map)
	{
		bn::regular_bg_ptr title_bg = bn::regular_bg_items::main_bg.create_bg(0, 0);
		bn::regular_bg_ptr title_text = bn::regular_bg_items::placeholder_title_screen.create_bg(0, 0);
		const bn::bg_palette_item& palette_item = bn::regular_bg_items::placeholder_title_screen.palette_item();
		bn::bg_palette_ptr bg_palette = title_bg.palette();
		bg_palette.set_colors(palette_item);

		while (!bn::keypad::start_pressed() && !bn::keypad::a_pressed())
		{
			map.randomize();
			bn::core::update();
		}
		return map;
	}

	void game_scene(dungeonmap map)
	{
		bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));


		creature player = map.creatures[0];

		bn::unique_ptr<bg_map> bg_map_ptr(new bg_map());

		bn::affine_bg_map_item bg_map_item(bg_map_ptr->cells[0], bn::size(bg_map::columns, bg_map::rows));
		bn::affine_bg_item bg_item(bn::affine_bg_items::tiles_tiles.tiles_item(), bn::affine_bg_items::tiles_tiles.palette_item(),
			bg_map_item);
		bn::affine_bg_ptr bg = bg_item.create_bg(0, 0);
		bn::affine_bg_map_ptr bg_map = bg.map();
		bn::regular_bg_ptr back_bg = bn::regular_bg_items::main_bg.create_bg(0, 0);
		bn::sprite_ptr compass = bn::sprite_items::compass.create_sprite(90, 60);
		bn::sprite_ptr needle = bn::sprite_items::needle.create_sprite(90, 60);
		bn::sprite_ptr portal = bn::sprite_items::portal_2.create_sprite(-16, 10);
		bn::sprite_ptr portal_mirror = bn::sprite_items::portal_2.create_sprite(16, 10);
		portal.set_visible(false);
		portal_mirror.set_visible(false);

		short current_bg = 0;

		bg.set_priority(2);

		back_bg.set_priority(3);

		needle.set_bg_priority(0);

		compass.set_bg_priority(1);

		portal.set_bg_priority(3);

		portal_mirror.set_bg_priority(3);

		map.generate_floor();
		bg_map_ptr->render(map);
		bg_map.reload_cells_ref();

		bn::fixed needle_angle = needle.rotation_angle();

		bn::music_items::holy_light_in_infernal_lands.play();

		while (player.current_health != 0)
		{
			if (bn::keypad::up_pressed())
			{
				short x = player.x;
				short y = player.y;
				map.move(0, true);
				player = map.creatures[0];
				bg_map_ptr->reset();
				bg_map.reload_cells_ref();
				bg_map_ptr->render(map);
				if (x != player.x || y != player.y)
				{
					if (current_bg == 0)
					{
						back_bg = bn::regular_bg_items::mirror_bg.create_bg(0, 0);
						current_bg = 1;
					}
					else
					{
						back_bg = bn::regular_bg_items::main_bg.create_bg(0, 0);
						current_bg = 0;
					}
				}
				portal = map.render_portal(portal, true);
				portal_mirror = map.render_portal(portal_mirror, false);

				map.randomize();
			}
			if (bn::keypad::down_pressed())
			{
				short x = player.x;
				short y = player.y;
				map.move(0, false);
				player = map.creatures[0];
				bg_map_ptr->reset();
				bg_map.reload_cells_ref();
				bg_map_ptr->render(map);
				if (x != player.x || y != player.y)
				{
					if (current_bg == 0)
					{
						back_bg = bn::regular_bg_items::mirror_bg.create_bg(0, 0);
						current_bg = 1;
					}
					else
					{
						back_bg = bn::regular_bg_items::main_bg.create_bg(0, 0);
						current_bg = 0;
					}
				}
				portal = map.render_portal(portal, true);
				portal_mirror = map.render_portal(portal_mirror, false);

				map.randomize();
			}
			if (bn::keypad::right_pressed())
			{
				map.turn(0, true);
				player = map.creatures[0];
				bg_map_ptr->reset();
				bg_map.reload_cells_ref();
				bg_map_ptr->render(map);
				if (current_bg == 0)
				{
					back_bg = bn::regular_bg_items::mirror_bg.create_bg(0, 0);
					current_bg = 1;
				}
				else
				{
					back_bg = bn::regular_bg_items::main_bg.create_bg(0, 0);
					current_bg = 0;
				}
				if (needle_angle + 90 >= 360)
				{
					needle.set_rotation_angle(0);
					needle_angle = needle.rotation_angle();
				}
				else
				{
					needle.set_rotation_angle(needle_angle + 90);
					needle_angle = needle.rotation_angle();
				}
				portal = map.render_portal(portal, true);
				portal_mirror = map.render_portal(portal_mirror, false);

				map.randomize();
			}
			if (bn::keypad::left_pressed())
			{
				map.turn(0, false);
				player = map.creatures[0];
				bg_map_ptr->reset();
				bg_map.reload_cells_ref();
				bg_map_ptr->render(map);
				if (current_bg == 0)
				{
					back_bg = bn::regular_bg_items::mirror_bg.create_bg(0, 0);
					current_bg = 1;
				}
				else
				{
					back_bg = bn::regular_bg_items::main_bg.create_bg(0, 0);
					current_bg = 0;
				}
				if (needle_angle - 90 < 0)
				{
					needle.set_rotation_angle(270);
					needle_angle = needle.rotation_angle();
				}
				else
				{
					needle.set_rotation_angle(needle_angle - 90);
					needle_angle = needle.rotation_angle();
				}
				portal = map.render_portal(portal, true);
				portal_mirror = map.render_portal(portal_mirror, false);

				map.randomize();
			}
			if (map.rooms[player.x + (player.y * map.columns)] == 4)
			{
				map.generate_floor();
				bg_map_ptr->reset();
				bg_map.reload_cells_ref();
				bg_map_ptr->render(map);
			}
			bn::core::update();
		}
	}
}

int main()
{
	bn::core::init();
	BN_LOG("started");
	dungeonmap map;

	while (true)
	{
		map = titlescreen(map);

		game_scene(map);

		bn::core::update();
	}
}
