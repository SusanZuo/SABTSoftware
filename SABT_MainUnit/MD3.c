/**
 * @file MD3.c
 * @brief Mode 3 code - Animal Game
 * @author Nick LaGrow (nlagrow)
 * @author Alex Etling (petling)
 * @author Kory Stiger (kstiger)
 * @author Poornima Kaniarasu (pkaniara)
 */

#include "Globals.h"
#include "Modes.h"
#include "audio.h"
#include "common.h"
#include "letter_globals.h"

int md3_current_state, md3_prev_state = 0;
static int game_mode = 0;
char md3_last_dot, last_cell, expected_dot;
int mistakes = 0;


char *animal_list[11] = {"bee", "camel", "cat", "cow", "dog", "horse",
    "hyena", "pig", "rooster", "sheep", "zebra"};

int animal_used_list[11] = {0,0,0,0,0,0,0,0,0,0,0};


/**
 * @brief Based off of the internal timer (TCNT1) - we generate
 *        a psuedo-random number. Turn that into a number from 1 - 11
 *        which corresponds to 1 of 11 different animals. Check which
 *        animals have been played already to be sure to play all of
 *        the different animals before repeating the list.
 * @return int - number between 1 - 11 corresponding to the animal file to play
 */
int choose_animal()
{
  int num = TCNT1;
  int i;

  num *= PRIME;
  num = (abs(num) % 11);

  char buf[10];
  sprintf(buf, "num=%i\r\n", num);
  PRINTF(buf);

  while(animals_used_list[num])
  {
    num = TCNT1;
    num *= PRIME;
    num = (abs(num) % 11);
  }

  animals_used_list[num] = 1;

  for(i = 0; i < 11; i ++)
  {
    sprintf(buf, "arr=%i, ",animals_used_list[i] );
    PRINTF(buf);
  }

  TX_NEWLINE_PC;
  sprintf(buf, "cnt=%i", animals_used+1);
  PRINTF(buf);
  TX_NEWLINE_PC;

  // increment animals_used until we've used all 11 animals then reset everything
  animals_used++;
  if (animals_used == 11)
  {
    animals_used = 0;
    for (i = 0; i < 11; i++)
      animals_used_list[i] = 0;
  }

  return num;
}

void md3_reset(void)
{
  md3_current_state = 0;
  md3_last_dot = 0;
  mistakes = 0;
}

/**
 * @brief  Step through the main stages in the code.
 *         Note : We need 2 STATE_REQUEST_INPUT states because request_to_play_mp3_file
 *         cannot handle 2 calls in quick succession.
 * @return Void
 * @TODO record MD3INT.MP3
 */
void md3_main(void)
{
  char animal_file[16];
  char animal_sound[16];
  char spell_letter[8];
  switch(md3_current_state)
  {
    case STATE_INITIAL:
      play_mp3("MD3_","INT"); // Welcomes and asks to choose a mode A or B
	  game_mode = 0;
      md3_current_state = STATE_SELECT_MODE; //STATE_REQUEST_INPUT1;
      animals_used = 0;
      got_input = false;
      break;

    case STATE_REQUEST_INPUT1:
	  if (game_mode == 1) play_mp3("MD3_","PLSA");
	  else if (game_mode == 2) play_mp3("MD3_","PLSB");
      length_entered_word = 0;
      current_word_index = 0;
      animal = animal_list[choose_animal()];
      md3_current_state = STATE_REQUEST_INPUT2;
      break;

    case STATE_REQUEST_INPUT2:

      if (game_mode == 1) play_mp3(NULL,animal);
      else if (game_mode == 2) { sprintf(animal_file, "N%s", animal);
	  play_mp3(NULL,animal_file);}

      md3_current_state = STATE_WAIT_INPUT;
      break;

    case STATE_WAIT_INPUT:
	 
      if(got_input)
      {
        got_input = false;		
        md3_current_state = STATE_PROC_INPUT;
      }
      break;

    case STATE_PROC_INPUT:
      // set entered_letter in valid_letter(), but return 1 or 0
	  
      if (last_cell == 0)
      {
        md3_current_state = STATE_READ_ENTERED_LETTERS;
      } else if (valid_letter(last_cell))
      {
        char buf[16];
        sprintf(buf, "%c", entered_letter);
        md3_current_state = STATE_CHECK_IF_CORRECT;
		if (!game_mode)
	    {
		if (entered_letter == 'a') game_mode = 1;
	    else if (entered_letter == 'b') game_mode = 2;
		else {
		    md3_current_state = STATE_WAIT_INPUT;
			break;
		}
		md3_current_state = STATE_REQUEST_INPUT1;
	    }
		play_mp3(NULL,buf);
      } else 
      {
	    mistakes = mistakes+1;
		PRINTF("mistake_inv");
		 play_mp3(NULL,"INVPAT");
		if (mistakes == 3){
			md3_current_state = STATE_WORD_HINT;
		}
		else if (mistakes >= 6){
			md3_current_state = STATE_LETTER_HINT;
		}
        else md3_current_state = STATE_READ_ENTERED_LETTERS;
      }
      break;

    case STATE_READ_ENTERED_LETTERS:
      if(length_entered_word > 0)
      {
        char buf[16];
        sprintf(buf, "%c", animal[current_word_index]);
        play_mp3(NULL,buf);
        current_word_index++;
      }

      if (current_word_index == length_entered_word)
      {
        md3_current_state = STATE_WAIT_INPUT;
        current_word_index = 0; 
      }
      break;

    case STATE_CHECK_IF_CORRECT:
      if (animal[length_entered_word] == entered_letter)
      {
        length_entered_word++;

        if (length_entered_word != strlen(animal))
          md3_current_state = STATE_CORRECT_INPUT;
        else
          md3_current_state = STATE_DONE_WITH_CURRENT_ANIMAL;
      } else 
      {
        md3_current_state = STATE_WRONG_INPUT;
      }
      break;

    case STATE_WRONG_INPUT:
	  
      play_mp3(NULL,"no");
	  mistakes = mistakes + 1;
	  PRINTF("mistakes");

	  if (mistakes == 3){
			md3_current_state = STATE_WORD_HINT;
	  }
	  else if (mistakes >= 6){
			md3_current_state = STATE_LETTER_HINT;
	  }
      else md3_current_state = STATE_READ_ENTERED_LETTERS;
      break;

    case STATE_CORRECT_INPUT:
	  mistakes = 3;
      play_mp3(NULL,"good");	  
      md3_current_state = STATE_WAIT_INPUT;
      break;

    case STATE_DONE_WITH_CURRENT_ANIMAL:
	  mistakes = 0;
	  play_mp3(NULL,"good");
	  play_mp3(NULL,"nc_wrk");
	  if (game_mode == 1) {
      	for (int count = 0; count < strlen(animal); count++) {
			sprintf(spell_letter,"%c",animal[count]);
			play_mp3(NULL,spell_letter);
		}  	  	
	  }
	  md3_current_state = STATE_PLAY_SOUND; 
      break;

    case STATE_SELECT_MODE:
	  play_mp3("MD3_","MSEL");
	  md3_current_state = STATE_WAIT_INPUT;
	  break;
 
    case STATE_PLAY_SOUND:
	  sprintf(animal_sound, "N%s", animal);
	  play_mp3(NULL,animal_sound);
	  if (game_mode == 2){
		  play_mp3("MD3_","SAYS");
		  play_mp3(NULL,animal);
		  }
	  md3_current_state = STATE_REQUEST_INPUT1;
	  break;
	case STATE_PROMPT:
	  break;

    case STATE_WORD_HINT:
	  play_mp3("PLS_","WRT");
	  sprintf(animal_sound, "N%s", animal);
	  play_mp3(NULL,animal_sound);
	  for (int count = 0; count < strlen(animal); count++) {
			sprintf(spell_letter,"%c",animal[count]);
			play_mp3(NULL,spell_letter);
	  }
	  md3_current_state = STATE_WAIT_INPUT;  	  
	  break;

	case STATE_LETTER_HINT:
	  play_mp3("PLS_","WRT");
	  char let[3];
      sprintf(let, "%c", animal[length_entered_word]);
	  PRINTF(let);
      play_mp3(NULL,let);
	  play_mp3(NULL,"press");	  
	  md3_current_state = STATE_BUTTON_HINT;
	  break;

	case STATE_BUTTON_HINT:
      curr_button += 1;
      char bits = get_bits_from_letters(animal[length_entered_word]);
      char curr_bit = (bits >> (CHARTOINT(curr_button) - 1)) & 1;
      //get the bits for each depending on button count - and play sound if bit is set
      if(curr_bit && curr_button >= '1' && curr_button <= '6'){
        play_requested_dot(curr_button);
      }

      if(CHARTOINT(curr_button) == NUM_BUT)
      {
        md3_current_state = STATE_WAIT_INPUT;
        curr_button = '0'; 
      }
      break;
  }
}
/**
 * @brief Handle left scroll button presses in mode 3
 * @return Void
 */

void md3_call_mode_left(void)
{
 	//replays the animal name or sound in accordance with submode
	md3_current_state = STATE_REQUEST_INPUT2;
}
/**
 * @brief Handle right scroll button presses in mode 3
 * @return Void
 */

void md3_call_mode_right(void)
{
// skips the animal 
 if (md3_current_state != STATE_PROMPT) md3_prev_state =  md3_current_state;
 play_mp3("MD3_","SKIP");
 md3_current_state = STATE_PROMPT;
}
/**
 * @brief Handle enter button presses in mode 3
 * @return Void
 */
void md3_call_mode_yes_answer(void)
{
  if (md3_current_state == STATE_PROMPT) md3_current_state = STATE_REQUEST_INPUT1;	
}

/**
 * @brief Handle exit buton pressed in this mode
 * @param Void 
 * @return Void
 */
void md3_call_mode_no_answer(void)
{  
   if (md3_current_state == STATE_PROMPT)
   {
	  md3_current_state = md3_prev_state;    
   }  
   else 
   {  
      play_mp3("MD3_","INT"); // Welcomes and asks to choose a mode A or B
	  game_mode = 0;
      md3_current_state = STATE_SELECT_MODE; 
      animals_used = 0;
	  mistakes = 0;
      got_input = false;
   }
}

/**
 * @brief Set the dot the from input
 * @param this_dot the entered dot
 * @return Void
 */
void md3_input_dot(char this_dot)
{
  md3_last_dot = this_dot;
  play_requested_dot(md3_last_dot);
}

/**
 * @brief handle cell input
 * @param this_cell the entered cell
 * @return Void
 */
void md3_input_cell(char this_cell)
{
  if(md3_last_dot != 0)
  {
    last_cell = this_cell;
    got_input = true;
  }
}
