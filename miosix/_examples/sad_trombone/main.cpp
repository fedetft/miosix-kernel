
#include "player.h"
#include "sad_trombone.h"

int main()
{
	ADPCMSound sound(sad_trombone_bin,sad_trombone_bin_len);
	Player::instance().play(sound);
}
