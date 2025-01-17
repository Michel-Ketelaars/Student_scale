//========================================================================================
// Student scale broncode versie 1.1 door Michel Ketelaars i.o.v. Welvaarts Weegsystemen
// Dit deel van de broncode is vrij te gebruiken en bedoeld om studenten kennis te laten 
// maken met de wereld van softwareontwikkeling. Lees voor meer info eerst het bijgevoegde
// README.TXT-bestand.
//=========================================================================================
// Deel 1: Bibliotheken
// Met de #include-instructie in de code hieronder wordt aangegeven welke bibliotheken  
// er worden gebruikt. Voor dit project maken we gebruik van een vijftal bibliotheken.  
// Drie hiervan zijn bedoeld voor de aansturing van specifieke hardware-onderdelen die we 
// gebruiken bij dit project. Met de #include-instructie maken we externe functies uit de 
// bibliotheken toegankelijk. Drie van de vijf bibliotheken zijn toegevoegd aan dit project 
// in de submap "libraries".  

#include <stdio.h>							//Bibliotheek voor *string operaties (gebruikte funties: sprintf)
#include <Wire.h> 							//Bibliotheek voor de *I2C bus (gebruikt door onderstaande display driver)
#include <LiquidCrystal_I2C.h>	//Display *driver bibliotheek (gebruikte object: LiquidCrystal_I2C)
#include <HX711.h>							//24 bit ADC (Analog naar Digital Converter) driver bibliotheek (object: HX711)
#include <Ticker.h>							//Bibliotheek voor timer fuctions (object Ticker)
#include <EEPROM.h>							//Bibliotheek voor opslag van data in EEPROM. 
																//Dit is 1 KB een geheugen die gebruikt wordt voor instellingen
																//welke bewaard blijven ook als de spanning wegvalt. 

// *strings zijn stukken tekst. In dit project gebruikt bij console-output en voor tekst op het display.
// *I2C is een serieel busprotocol dat in dit project wordt gebruikt voor het aansturen van het display.
// *driver is de naam van een bibliotheek die nodig is voor het aansturen van een specifiek stukje hardware.

//-----------------------------------------------------------------------------------------
// Deel 2: Constanten en definities
// Een enum-statement is een vorm van een variabeledefinitie die meestal maar een beperkt aantal 
// constante waardes vertegenwoordigt. De eerste waarde uit de lijst hieronder is 0. De 
// daaropvolgende constante waardes (zonder toevoeging van een =-teken) zijn steeds de voorgaande waarde + 1.
// Het is gebruikelijk om enum-waardes en constanten te schrijven in volledig hoofdletters. Ook geldt
// dat variabelen geen spaties mogen bevatten, vandaar het gebruik van de underscore (_) in de variabelenaam.

enum ePUSH_BUTTON {NONE_PRESSED = 0, YELLOW_PRESSED, GREEN_PRESSED, BLUE_PRESSED, START_UP};

// Bij de drivers voor het aansturen van de hardware zal bekend moeten zijn op welke pinnen deze 
// zijn aangesloten op het Arduino-board. Dit geven we aan door middel van constante waardes die 
// een pinnummer vertegenwoordigen. Als eerste zijn de alarm-leds. Dit is een zogeheten bi-color led  
// met twee aansluitingen voor de kleuren groen en rood, welke verbonden zijn met pin 5 en 6 van 
// de Arduino. De betekenis van de instructies const en int wordt verderop besproken.

//Alarm leds
const int LIGHT_GREEN_PIN = 6;
const int LIGHT_RED_PIN = 5;

// De HX711 is de naam van de ADC die we gebruiken om het analoge krachtopnemer-signaal 
// om te zetten in een digitale waarde waarmee de software kan rekenen. De ADC is 
// aangesloten op de I2C-bus op de pinnen 2 en 3 voor respectievelijk de data- en kloklijn.

	// I2C bus aansluitingen voor HX711 
const int LOADCELL_DAT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

// Voor de bediening van de weegschaal zijn er drie drukknoppen vervaardigd in de kleuren 
// blauw, geel en groen. Deze zijn respectievelijk aangesloten op de pinnen 14, 15 en 16.

	// Drukknop aansluitingen
const int SWITCH_YELLOW_PIN = 14;
const int SWITCH_GREEN_PIN = 15;
const int SWITCH_BLUE_PIN = 16;

// Met de #define instructies kennen we een vaste waarde toe aan een woord, zodat de 
// broncode beter leesbaar wordt en het direct duidelijk is wat er wordt bedoeld bij 
// een bepaalde instructie. Ook is het een handig middel om vanuit één punt 
// de controle te hebben over een waarde die op meerdere plaatsen in de code wordt gebruikt.
// Met de eerste twee onderstaande #defines wordt het aantal karakters en regels gedefinieerd 
// waaruit het display bestaat. De displaydriver kan namelijk ook worden gebruikt voor displays 
// die bestaan uit meerdere karakters en regels.

#define DISPLAY_CHARS 16        // Aantal karakters per regel op het display
#define DISPLAY_LINES 2         // Aantal regels op het display
#define DISPLAY_ADDRESS 0x27    // I2C adres van het display
#define TIMER_INTERVAL 100      // Aantal milliseconden tussen elke timer-interrupt (100 ms)
#define STR_FORMAT_BUF_SIZE 100 // Geheugenomvang tekst-string formateringen

//-----------------------------------------------------------------------------------------
// Deel 3: Functie declaraties
// In de programmeertalen C/C++ worden functies vooraf gedefinieerd als er in de broncode
// eerder naar verwezen wordt, voordat de functie wordt beschreven. Dit is ook het geval voor de
// twee onderstaande functies. Later in dit bestand wordt er invulling aan de functies gegeven.
// Als we dat punt hebben bereikt, zullen we uitleggen waar deze twee functies voor bedoeld zijn.
// Ook de opbouw van functies wordt later beschreven.

void timer_interrupt();
ePUSH_BUTTON get_push_button();

//-----------------------------------------------------------------------------------------
// Deel 4: Objecten
// Hieronder worden eerst drie C++ objecten aangemaakt, waarvan de structuur en werking 
// beschreven staan in de bibliotheken die we aan het begin hebben toegevoegd.
// De eerste is een object voor de ADC om de krachtopnemer uit te lezen.
// Als je de Ctrl-toets indrukt en met de muis de zogenaamde class name HX711 aanwijst,
// wordt deze blauw van kleur. Als je er vervolgens op klikt, kom je in het header 
// (extensie .h) bestand dat de class beschrijft. In deze header vind je de functies die je
// kunt aanroepen, nadat het object is aangemaakt.

HX711 scale;                                                                // Maak een object voor de ADC uitlezing
LiquidCrystal_I2C display(DISPLAY_ADDRESS, DISPLAY_CHARS, DISPLAY_LINES); 	// Maak een object voor het display
Ticker timer(timer_interrupt, TIMER_INTERVAL);							                // Maak een object voor de timer interrupt

// Bij het maken van het display object, genaamd [naam], worden enkele constante parameters 
// opgegeven. Hiermee weet de driver de eigenschappen van het aangesloten display, zoals 
// beschreven bij de betreffende constanten.
// Ook bij het maken van het timer object worden twee parameters opgegeven. De eerste is de 
// naam van de functie die wordt aangeroepen op het moment dat de ingestelde tijd is doorlopen. 
// De tweede parameter is de waarde van deze tijd in milliseconden. De gebruikelijke naam voor 
// een functie die actief wordt als er een gebeurtenis plaatsvindt, heet interrupt.

//-----------------------------------------------------------------------------------------
// Deel 5: (Globale) variabele
// Variabelen zijn voorgedefinieerde geheugenplaatsen voor het opslaan van tijdelijke waarden.
// Binnen dit project gebruiken we er een aantal met verschillende types, die een 
// bepaalde betekenis hebben. De eerste is van het type long en is een variabele die een geheel getal 
// vertegenwoordigt. Dit kan zowel een positief als een negatief getal zijn. Het maximale 
// geheugen voor opslag van een long is 32 bits, wat neerkomt op een bereik van -2.147.483.648 
// tot 2.147.483.647.

long adc_sample;		// Variabele voor de opslag van een AD-waarde, afkomstig van de ADC

// Het tweede type variabele is float, en is tevens 32 bit groot. Dit type variabele vertegenwoordigt 
// een decimaal getal in de vorm van een soort wetenschappelijke notatie. Zes tot zeven significante 
// cijfers kunnen hierin worden opgeslagen, met een bereik van ongeveer plus of min 38 nullen 
// voor of na de komma.

float gross;                // Opslag voor bruto gewicht
float net;                  // Opslag voor netto gewicht
float tare;                 // Opslag voor tarrawaarde
float calibration_weight;   // Opslag voor kalibratiewicht waarmee de weegschaal wordt afgesteld

// Het derde type is de bool-variabele. Dit representeert een binaire waarde, dus 1 of 0, ook
// wel vaak vertaald als true of false. Hoewel er maar twee mogelijkheden zijn, wordt de waarde 
// wel opgeslagen in één byte, dus acht bits, en niet in 1 bit, wat je misschien zou vermoeden.

bool net_view;             // Weergavemode, netto of bruto weergave
bool calibration_mode;     // Kalibratiemode voor het afstellen van de weegschaal
bool tared;								 // Geeft aan of er getareerd is

// Het vierde type dat je hieronder ziet is de char-instructie. Hierin kan een byte aan
// data worden opgeslagen, dus in het geval van tekst één enkel karakter. Dit type bestaat vaak uit 
// een serie aaneengesloten bytes om stukken tekst (vaak strings genoemd) in op te slaan. Dit is zo 
// ook gebeurd in het voorbeeld hieronder. Met de blokhaken [] wordt aangegeven dat het om een serie gaat
// waarbij in dit geval het geheugen gevuld wordt met de tekst erachter. De instructie `const` voorafgaand 
// aan `char` wil zeggen dat het om een constante gaat, dus met andere woorden de inhoud hiervan wordt 
// niet aangepast door de broncode.

const char gross_txt[] = "Gross:"; // Displaytekst voor weergave bij bruto gewicht
const char net_txt[] = "Net:";     // Displaytekst voor weergave bij netto gewicht

char str_format_buffer[STR_FORMAT_BUF_SIZE];// De debuginformatie wordt via de seriële poort verstuurd. 

// Het vijfde variabele type is int en qua betekenis gelijk aan het type long, maar dan 16 i.p.v. 32 bit.
// Hiermee wordt het bereik van dit type -32.768 tot 32.767. De extra toevoeging volatile mag voor nu 
// even vergeten worden. Het is een toevoeging voor de compiler voor optimalisatiebescherming, 
// die nodig is wanneer de variabele ook binnen de interrupt wordt gebruikt.

volatile int AdcTimer;    // Timer teller variabele, waarde wordt iedere 100 ms verlaagd in de timer interrupt
int pressed_time_counter; // Timer teller voor het indrukken van de groene knop om naar calibratie mode te gaan

// Een type struct zoals hieronder is een bijzonder variabele en beschrijft een groep samenhangende variable in 
// een aaneengesloten geheugenstuk. De variable in deze struct zijn configuratie-instellingen welke we opslaan 
// in de het EEPROM geheugen waarmee de Arduino Nano is uigerust. Kenmerkend voor het Eerprom geheugen is 
// dat de waardes bewaard blijven ook als de spanning wegvalt. Hierdoor is het uitermate geschikt voor het 
// opstaan van deze configuratie instelling.

struct sConfig {
  char name[8];    			// String van 7 karakters + 1 voor '\0' (null-terminator)
  float weigh_factor;   // Factor die gebruikt wordt om een AD-waarde om te zetten naar de gewichtswaarde
  long zero_ad_value;  // Variabele voor de opslag van de Zero-waarde, gebruikt om de weegschaal op nul te zetten.
  int bootCount;   			// Een teller voor het aantal opstartkeren
};

sConfig myConfig; // Kopie werkgeheugen voor de configuratie-instellingen die zijn opgeslagen in de EEPROM

//-----------------------------------------------------------------------------------------
// Deel 6: Basisfunctie setup()
// Bij Arduino-projecten zijn er twee basisfuncties, namelijk setup() en loop(). De setup wordt hieronder
// beschreven en is de functie die eenmalig als eerste wordt aangeroepen nadat je programma opstart.
// In de functie configureren en initialiseren we de hardware die is aangesloten op het bord.
// Functies kenmerken zich door de haakjes die erachter staan, voorafgaand aan een type 
// variabele welke de functie retourneert op het moment dat deze wordt afgesloten. Tussen de haakjes 
// kunnen parameters worden gezet die worden meegegeven met de functie. Bij deze setupfunctie 
// is dit niet het geval. Ook retourneert de functie geen waarde terug, in dat geval schrijven we
// het type void, wat in het Engels 'leeg' betekent. De inhoud van de functie is alles wat 
// tussen de accolades {} staat.

void setup()
{
	display.begin();   // Functie-aanroep voor de initialisatie van het display
	display.noBlink(); // Hiermee geven we aan dat we geen cursor willen zien op het display.

	// Binnen het object scale (van de class HX711) maken we met de functie begin() kenbaar
	// op welke pinnen de I2C-bus van de ADC (voor het uitlezen van de krachtopnemer) is aangesloten.

	scale.begin(LOADCELL_DAT_PIN, LOADCELL_SCK_PIN);
	
	// Voor debugdoeleinden maken we gebruik van seriële output over de comport waarmee ook 
	// de Arduino wordt geprogrammeerd. De seriële bussnelheid (ook wel baudrate genoemd) wordt
	// hier gespecificeerd en als parameter meegestuurd bij de initialisatie van de seriële poort.
	// Het opgegeven getal betekent het aantal bits dat per seconde wordt overgedragen, dus in dit geval 9600. 
	// De debuginformatie kan worden ingezien via de Serial Monitor vanuit het menu Tools in de Arduino IDE.
	// Voor het object Serial zijn hierboven geen #include-bibliotheek toegevoegd, dit is niet nodig omdat 
	// dit object onderdeel is van de zogeheten Arduino Core Libraries die altijd toegankelijk zijn.

  Serial.begin(9600);

	// De pinMode-functie is ook onderdeel van de Arduino Core. Met deze functie wordt beschreven
	// wat het doel is van bepaalde pinnen waarop de hardware is aangesloten. Zo wordt van een opgegeven
	// pinnummer aangegeven of het een input- of outputpin betreft. In het geval van de drukknoppen
	// hebben we te maken met inputsignalen, wat hieronder kenbaar wordt gemaakt.

	pinMode(SWITCH_YELLOW_PIN,INPUT);
	pinMode(SWITCH_GREEN_PIN,INPUT);
	pinMode(SWITCH_BLUE_PIN,INPUT);

	// Naast bovengenoemde inputpinnen zijn er ook twee outputpinnen waar we de rood/groene
	// bi-color LED op hebben aangesloten. De nummers hiervan, met de outputdefinitie, worden 
	// hieronder kenbaar gemaakt.

  pinMode(LIGHT_GREEN_PIN,OUTPUT);
  pinMode(LIGHT_RED_PIN,OUTPUT);

	// Initieel zetten we de outputpinnen op een bepaalde waarde, hoog of laag. Dit doen we 
	// met de corefunctie digitalWrite. In het geval van de onderstaande LEDs resulteert dit 
	// in de LED aan of uit.

  digitalWrite(LIGHT_GREEN_PIN,LOW);  // Zet de groene led aan bij opstarten
  digitalWrite(LIGHT_RED_PIN,HIGH);   // Zet de rode led uit bij opstarten.

	// Hoewel we het hieronder over outputpinnen hebben, moeten deze voor de correcte werking
	// toch op een hoge waarde worden ingesteld. Dit geeft een zogeheten pull-up werking, waarbij 
	// de pin hoog is in de passieve toestand en laag wordt op het moment dat je de aangesloten 
	// drukknop indrukt.

  digitalWrite(SWITCH_BLUE_PIN,HIGH);
  digitalWrite(SWITCH_YELLOW_PIN,HIGH); 
  digitalWrite(SWITCH_GREEN_PIN,HIGH); 
	
	tare = 0;                 // Bij opstarten worden de gewichtwaardes initieel op nul gezet.
	gross = 0;
	net = 0;
	adc_sample = 0;
	pressed_time_counter = 0;
	tared = false;						// Na opstarten is er geen tarra in het geheugen
	net_view = false;         // Na opstarten is de weergave altijd bruto gewicht
	calibration_mode = false; // Na opstarten is de mode altijd normaal (geen kalibratiemode)
	calibration_weight = 500; // Na opstarten is het standaard kalibratiewicht ingesteld op 500 gram
	
	EEPROM.get(0, myConfig); 	// Lees het EEPROM geheugen in kopieer dit naar het lokale geheugen

	// Controleer of het programma voor de eerste keer opstart. De waarde van de string is 
	// dan niet gelijk aan de tekst "Student". Deze beschrijven we namelijk pas na de eerste 
	// keer opstarten. De functie strcmp() staat voor string compare en is een functie om de 
	// inhoud van twee stukken tekst te vergelijken.
	
	// Om naar een element binnen een struct te verwijzen, gebruik je de punt-notatie zoals hieronder
	// dus <variablenaam van struct>.<elementnaam binnen struct> => myConfig.name

	if(strcmp(myConfig.name, "Student")) //Als tekst niet gelijk is (ofwel eerste keer opstarten)
	{		
		// Enkele variabelen hebben we vooraf gedefinieerd met een vooraf ingestelde waarde.
		// Dit is gedaan voor de weegfactor en de zero AD-waarde aftrek. De waarden hieronder
		// zijn proefondervindelijk vastgesteld en zorgen ervoor dat de weegschaal ongeveer op 
		// nul kilogram staat na het opstarten. De factorwaarde is zo bepaald dat wanneer je de 
		// weegschaal belast met een object, het displaygewicht goed overeenkomt met het werkelijk  
		// gewicht van het object.
	
		strcpy(myConfig.name, "Student");

		myConfig.zero_ad_value = 810500; // Dit is ongeveer de AD-waarde van een onbelaste weegschaal
		myConfig.weigh_factor = 0.00228; // De eenheid van deze factor is gram/AD-waarde 
																		 // (bepaald met specificaties van de krachtopnemer)

		myConfig.bootCount = 0;					 // Teller voor aantal keren opgestart op nul zetten

		EEPROM.put(0, myConfig); //Schrijf de EEPROM met bovenstaande default configuratie waarden

		Serial.println("De default configuratie is opgeslagen in de EEPROM");
	}

	AdcTimer = 5;             // Zet de waarde voor de interrupt timer op 100 ms (= 5 x 20ms)
	timer.start();            // Start de timer-interrupt, van dit punt af zal iedere 100 ms de opgegeven 
	                          // functie timer_interrupt() worden aangeroepen
	
	// Schrijf deze tekst als eerste uit over de seriële poort. 
	// Direct na opstarten is deze te zien via de seriële monitor.
	Serial.println("========================================="); 
	Serial.println("Welvaarts Studenten Weegschaal versie 1.1"); 
	Serial.println("========================================="); 

	myConfig.bootCount++;			// Teller voor aantal keren met waarde 1 ophogen

	// De offsetof() is een zogeten marco en dus geen functie. Aan de instuctie regel zelf kun je dit
	// overigens niet zien, dus had ook een fuctie kunnen zijn. De macro simplificeerd de schrijfwijze om 
	// de geheugen offset van een element binnen een struct te bepalen. Het element in het geval hieronder
	// is bootCount en de struct is sConfig.
	
	int offset = offsetof(sConfig, bootCount); // Geeft geheugen offset van bootCount binnen de struct sConfig
	EEPROM.put(offset, myConfig.bootCount);    // Schrijf de "verhoogde teller waarde" ook terug in de EEPROM
	

	char str_buffer[15];
	dtostrf((myConfig.weigh_factor*1000), 6, 2, str_buffer); //Weegfactor formateren in milligram/AD-waarde
	Serial.println("Configuratie instellingen uit EEPROM:"); 
	sprintf(str_format_buffer, "Aantal opstartkeren teller: %d", myConfig.bootCount);
	Serial.println(str_format_buffer); 
	sprintf(str_format_buffer, "AD-waarde bij nul kilogram: %ld", myConfig.zero_ad_value);
	Serial.println(str_format_buffer); 
	sprintf(str_format_buffer, "Weeg-factor [mg/AD-waarde]: %s", str_buffer);
	Serial.println(str_format_buffer); 
	Serial.println("-----------------------------------------"); 
}

//-----------------------------------------------------------------------------------------
// Deel 7: Basisfunctie loop()
// De tweede Arduino-basisfunctie, loop(), wordt hieronder beschreven. De functie heet loop 
// omdat deze oneindig wordt doorlopen. Als de functie ten einde komt, zal de onderliggende 
// Arduino Core de functie direct opnieuw aanroepen. In deze functie zit alle logica voor het 
// besturen van de weegschaal. Het gewicht wordt bepaald na het passeren van een ingestelde tijd, 
// het display wordt aangestuurd en de drukknoppen worden gemonitord. Bij drukknop-events zullen 
// hier ook acties worden ondernomen.

void loop()
{
	// Bij een functie beginnen we meestal met het initialiseren van lokale variabelen. Het begrip lokaal
	// betekent dat deze variabelen niet buiten de functie toegankelijk zijn. Dit is in tegenstelling tot
	// de globale variabelen en objecten die we aan het begin van dit bestand hebben aangemaakt.
	// Het variabele type bool hebben we eerder gezien, echter de voorafgaande instructie static is 
	// nieuw. Met static geven we aan dat de waarde van de variabele onthouden wordt wanneer de functie wordt 
	// verlaten. Bij het opnieuw aanroepen van deze functie is de waarde dus gelijk aan de waarde toen 
	// de functie werd verlaten. De waarde-toekenning met het = teken is alleen van toepassing voor 
	// de allereerste aanroep van de functie en is hiermee een initiële waarde.
	
	static bool prev_scale_found = true;  // Dit betekent dat de weegschaal is gevonden bij de vorige keer 
                                      	// dat deze functie werd aangeroepen. We bedoelen hier de HX711 
                                      	// ADC met krachtopnemer.
	
	bool scale_found;                     // Gelijk aan voorgaande variabele, maar nu voor de actuele 
	                                      // toestand, die bepaald wordt in deze functie-aanroep.
	                                      
	ePUSH_BUTTON current_state;           // Deze enum-variabele geeft de toestand (state) aan waarin de 
	                                      // drukknoppen zich bevinden.
	
	static ePUSH_BUTTON previous_state = START_UP; // Dit is de toestand van de drukknoppen bij de vorige 
	                                               // functie-aanroep.
	
	char str_display_weight[DISPLAY_CHARS+1];  // Hier reserveren we werkgeheugen waarin we het gewicht 
	                                           // voor het display opslaan in de vorm van tekst.
	
	char display_line_two[DISPLAY_CHARS+1];    // Ook voor het formatteren van de tweede regel tekst voor 
	                                           // het display reserveren we hier geheugen.
		
	// Vanaf hier wordt de functiecode uitgevoerd. We beginnen met het bepalen van de huidige toestand
	// van de drukknoppen. Hiervoor is een aparte functie gemaakt die verderop wordt beschreven.

	current_state = get_push_button();  // Bepaal de huidige drukknoptoestand

//-----------------------------------------------------------------------------------------
// Deel 8: Basic flow control statements
// Dit zijn instructies in C zoals o.a.: if, else, switch, case, break die hieronder zijn gebruikt.
// Met deze functies geef je sturing aan de volgorde waarin instructies worden uitgevoerd. De meest
// gebruikte is de zogeheten if-instructie. Met deze instructie vergelijk je variabelen en/of waarden op hun
// inhoud met elkaar. Het resultaat is altijd een boolean, oftewel waar (true) of niet waar (false).
// In geval van true wordt de code uitgevoerd die direct onderliggend is, dus tussen de accolades {}.
// Mocht de uitkomst false zijn, wordt het hele blok overgeslagen en gaat de verwerking op dat punt verder.
// Om de verwerkingvolgorde bij het aanschouwen van de code beter inzichtelijk te maken, springen
// we naar rechts met de instructies binnen de accolades {}.

	if(current_state != previous_state) // Lees dit als volgt: 
	{                                   // Als de huidige drukknoptoestand niet gelijk is aan de vorige toestand?
	                                    // != lees je als "niet gelijk" en == lees je als "gelijk"
	  
    // Hier kom je alleen als het antwoord op bovenstaande vraag beantwoord wordt met: ja

		if(current_state != NONE_PRESSED)  // Is de huidige drukknoptoestand niet gelijk aan de toestand: "geen drukknop ingedrukt"?
		{                                  // Of eenvoudiger gezegd: Is er een knop ingedrukt?
		
	    // Hieronder volgt een tweede type flow control statement, te weten switch. Bij een switch-
	    // instructie toets je een variabele aan een vaste waarde. In onderstaand geval zijn er drie mogelijke
	    // uitkomsten, welke terug te vinden zijn achter de instructie case, die onderdeel uitmaakt van de 
	    // switch-instructie.
	
	    // Omdat we op deze plek terecht zijn gekomen, weten we dat er een drukknop is ingedrukt. De vraag is 
	    // welke knop, zodat we bij de juiste case uitkomen en vervolgens de bijbehorende tekst naar de seriële debug 
	    // console versturen.

			switch(current_state)   // Hier staat de variabele die we willen toetsen, in dit geval de huidige drukknoptoestand
			{                             
		    case YELLOW_PRESSED:  // Als uitkomst is dat de gele knop wordt ingedrukt, dan onderstaande uitvoeren
		        Serial.println("Yellow button pressed");  // Tekst sturen naar seriële output
		        break;                            
		
        // Deze bovenliggende break-instructie is ook onderdeel van de voorgaande case-instructie.
        // Hiermee geef je aan dat de case ten einde is. Mocht je deze vergeten of bewust weglaten, 
        // dan worden hierna ook de instructies van de eerstvolgende case uitgevoerd. 
        // Bij het uitvoeren van de break-instructie springt het programma verder naar de eindaccolade 
        // van de switch-instructie.
					
				case GREEN_PRESSED:		// Als groene knop wordt ingedrukt
					Serial.println("Green button pressed");		
					break;
					
				case BLUE_PRESSED:		// Als blauwe knop wordt ingedrukt
					Serial.println("Blue button pressed"); 		
					break;
				
				// Een bijzonder uitkomst die we naast de verschillende cases gebruiken in de default-instructie. Dit is het 
				// punt waar naartoe gesprongen wordt indien geen van alle case-uitkomsten gelijk is aan de getoetste variabele.
				// We gebruiken deze nu niet, althans er zitten geen instructies onder, maar hebben deze voor de volledigheid 
				// van deze uitleg wel toegevoegd.
				
				default:             // Als zowel de groene, blauwe als gele knop niet zijn ingedrukt. Als je de logica en
				    break;           // mogelijke uitkomsten voor de current_state bestudeert, zul je zien dat dit niet kan voorkomen.
				
			} // Eind van switch-accolade (is het terugkeerpunt na break-instructies hierboven)

			pressed_time_counter = 0; // Reset de timer-counter, welke gebruikt is om met lang indrukken van de
			                          // groene knop naar de kalibratiemode te gaan.
		}
		else 
		{ 	// Een if-instructie kan gevolgd worden door een else-instructie. Hier komt het programma terecht
        // als het resultaat van de bijbehorende if-instructie false is, ofwel het antwoord op de vraag "Is er een knop ingedrukt?" is nee.
        // Dus in dit geval: Als er geen knop ingedrukt is, gaat het programma verder hier.

			if(previous_state != START_UP) // Wordt deze functie niet voor de eerste keer aangeroepen?
			{                              // Dit is de vraag die achter deze vergelijking schuilgaat, omdat
				                             // alleen bij de eerste aanroep de waarde van previous_state gelijk is aan START_UP

		    // Hieronder een compactere schrijfwijze die vrijwel gelijk is aan de eerder beschreven
		    // switch-instructie, maar nu voor het loslaten van een drukknop.

				switch(previous_state)
				{
					case YELLOW_PRESSED:	Serial.println("Yellow button released"); break; // De gele knop is losgelaten
					case GREEN_PRESSED:		Serial.println("Green button released");	break; // De groene knop is losgelaten
					case BLUE_PRESSED:		Serial.println("Blue button released"); 	break; // De blauwe knop is losgelaten
				}
			}
		} 
	
		if(!calibration_mode) // Als het systeem zich in normale mode bevindt
		{		
			int offset;
			
			display.setCursor(0,0); // Zet de cursor (de schrijfpositie op het display) op het eerste karakter van regel één.
			switch(current_state)
			{
				case NONE_PRESSED:										// Als er geen knop wordt ingedrukt wordt deze tekst op de
					display.print("Student Scale   ");  // eerste regel van het display gezet. Met de extra spaties aan
					break; 															// het einde overschrijf je eventueel oude tekst die er voorheen
																							// stond, dus deze niet weghalen.
				case YELLOW_PRESSED:	
					if(!tared)													// Als er geen tarra in het geheugen staat.
					{
						display.print("Tare request    ");// Als de gele tara knop wordt ingedrukt zetten we deze tekst op het
						tare = gross;											// display. Voor de tarrering slaan we het laatst berekende bruto
						net_view = true; 									// gewicht op als zijnde tarra gewicht. Daarnaast willen we
						tared = true;											// dat het netto gewicht getoond wordt op het display. (net_view)
					}	
					else
					{
						display.print("Reset tare      ");// Tekst voor op display bij resetten van tarra
						tare = 0;													// Haal tarra uit het geheugen
						net_view = false;									// Schakel terug naar bruto weergave
						tared = false;										// Geef aan dat tarra uit het geheugen is gehaald
					}
					break;
					
				case BLUE_PRESSED:										
					display.print("Zero request    ");	// Als de blauwe knop wordt ingedrukt komt deze tekst op het 
					tare = 0;														// display. De tara wordt gereset, mocht deze een waarde bevatten.
					myConfig.zero_ad_value = adc_sample;// De zero waarde zetten we gelijk aan de huidige ADC waarde. Dit 
																							// resulteert verderop bij de gewichtscalculatie in bruto 0 kg.

					offset = offsetof(sConfig, zero_ad_value); 	// Geeft geheugen offset van zero_ad_value binnen de struct
					EEPROM.put(offset, myConfig.zero_ad_value); // Schrijf de (aangepaste) AD waarde bij 0 kg terug in de EEPROM
					break;

				case GREEN_PRESSED:
					if(tared)														// Als er een tarrering in het geheugen staat	en
					{																		// de groene knop wordt ingedrukt zien we deze tekst.
						display.print("Toggle Gross/Net");// We schakelen hiermee tussen bruto en netto weergaven.
						net_view = !net_view;							// Het uitroepteken ! kun je lezen als niet. Dus bij deze boolean
						AdcTimer = 0;											// wordt de waarde omgetoggled van true naar false of andersom.
					}																		// Het resetten van de AdcTimer triggert/versneld een scherm update.
					else																
					{
					  display.print("No tare stored! "); // Display tekst als er geen tarrering in het geheugen staat	
					}
					break;
			}
		}
		else //Als het systeem zich in kalibratie mode bevind.
		{ 	
			// Het gedrag van de knoppen in kalibratie mode is anders dan in de gewone mode. Hieronder is dit uitgewerkt.
	    // We zien hier ook nieuwe notaties. Eén ervan is de notatie +=. Dit betekent "neem de waarde van de variabele
	    // ervoor en voeg de waarde erachter toe". Stel dat x = 2, dan wordt x += 3 en zal de waarde van x gelijk
	    // worden aan 5. Voor de notatie -= geldt hetzelfde, maar dan wordt er van de waarde afgetrokken.
	
	    // Verder zien we een vreemde notatie met ? en :. Dit is een beknopte vorm van een if-else statement. 
	    // De waarde voor de vergelijking vóór de ? is een boolean (true of false). Als de uitkomst true is, 
	    // wordt de waarde vóór de :, en als de uitkomst false is, wordt de waarde na de : uitgevoerd.
			
			switch(current_state)
			{
				case YELLOW_PRESSED:	// Als de gele knop wordt ingedrukt. Tel 100 op bij 
															// het huidige kalibratiegewicht als deze kleiner is
															// dan 1000 en tel 500 op indien groter of gelijk aan 1000
															
					calibration_weight += (calibration_weight<1000) ? 100 : 500; 
					
					// De maximale waarde voor het kalibratiegewicht is 5 kg, wat gelijk is aan de capaciteit van de krachtopnemer.
					// Als het resultaat van de berekening hoger is dan dit maximum, wordt het teruggezet naar 5 kg. (5000 g)

					if(calibration_weight > 5000) 
							calibration_weight = 5000;  
					
					break;

				case BLUE_PRESSED:	// Als de blauwe knop wordt ingedrukt. Trek 100 af van 
														// het huidige kalibratiegewicht als deze kleiner of gelijk 
														// is aan 1000 en trek 500 af indien groter dan 1000
										
					calibration_weight -= (calibration_weight<=1000) ? 100 : 500; 

					// De minimale waarde van het kalibratiegewicht is 100 g. Als het resultaat 
					// van de berekening lager is, wordt het kalibratiegewicht teruggezet naar dit minimum.

					if(calibration_weight < 100) 	
							calibration_weight = 100;

					break;

				case GREEN_PRESSED:		// Als de groene knop wordt ingedrukt voeren we de 
															// kalibratie uit en verlaten daarna de kalibratiemode

					adc_sample = scale.read(); // Lees een actueel gewichtssample uit de ADC

					// We voeren alleen een kalibratie uit als er een minimale belasting van ongeveer 50 gram  
					// ten opzichte van het nulpunt op de krachtopnemer is aangebracht.
					if((adc_sample - myConfig.zero_ad_value) > 23000) // +/-50gr
					{
						char str_buffer[50];	//Tijdelijk buffer voor het opslaan van 50 karakters
	
							// Aan de hand van de belasting en opgegeven kalibratiegewicht bepalen we gewichtsfactor
						myConfig.weigh_factor = calibration_weight / (adc_sample - myConfig.zero_ad_value);

							// Met onderstaande functie zetten we een float (decimaal getal) om naar een tekstnotatie
							// die wordt opgeslagen als een string in een char-buffer. De functie heeft vier parameters:
							// 1. Het float-getal,
							// 2. de minimale breedte,
							// 3. het aantal cijfers achter de komma,
							// 4. de string buffer waarin het resultaat wordt opgeslagen.

						dtostrf(myConfig.weigh_factor, 6, 5, str_buffer);

							// Aan het tekstresultaat van de voorgaande functie willen we voorafgaand nog wat toevoegen.
							// Hiervoor gebruiken we de sprintf functie uit de stdio.h bibliotheek. Dit is een complexere functie
							// die gebruik maakt van format specifiers, die te herkennen zijn aan de % toevoeging. In het geval van %s 
							// betekent dit dat een string wordt ingevoegd. De derde parameter, de string 'str_buffer', wordt ingevuld op 
							// de plaats van %s, wat resulteert in een samenvoeging van de twee teksten.
							
						sprintf(str_format_buffer, "Set weigh factor to: %s", str_buffer);

							// Opmerking: normaal gesproken zou je met sprintf direct floats kunnen formatteren 
							// en omzetten naar strings met de %f formatter. Dit zou de dtostrf() functie overbodig maken.
							// Echter, deze %f formatter werkt niet op Arduino Uno boards, daarom hebben we de bovenstaande
							// methode (met dtostrf) moeten toepassen.
														
						Serial.println(str_format_buffer);						// String output met factor naar serieel console

						int offset = offsetof(sConfig, weigh_factor); // Geeft geheugen-offset van weigh_factor binnen de struct
						EEPROM.put(offset, myConfig.weigh_factor); 	  // Schrijf de (aangepaste) weegfactor waarde ook terug in de EERPOM

            Serial.println("Leave calibration mode");
					} else { 
						Serial.println("Calibration load is too low!");
					}
					AdcTimer = 0; // Reset AdcTimer voor een versnelde schermupdate

					calibration_mode = false; // Zet mode terug om mormale mode
					break;
			}

			display.setCursor(0,1);		// Zet de cursor op de tweede regel
			
				// float type omzetten naar tekst string (lengte is 4 zonder decimalen)
			dtostrf(calibration_weight, 4, 0, str_display_weight); 

				// formateer string voor tweede regel op display
			sprintf(display_line_two, "%s g   Nul- T+", str_display_weight);
			
			display.print(display_line_two);	// Zet de tweede regel op het display
		}

		// Er is een wijziging in de drukknoptoestand gedetecteerd en erop geacteerd.
		// Voor de juiste werking in de volgende herhaling van deze functie, zetten we de 
		// huidige toestand van de drukknop over naar de vorige toestand.
		previous_state = current_state;
	}

	// Dit gedeelte van de code wordt uitgevoerd als de timer op nul staat.
	// Het terugtellen naar nul wordt geregeld door de timerinterruptfunctie,
	// die verderop in de code is gedefinieerd.
	
	if(!AdcTimer) // Als 500 ms verstreken zijn sinds de laatste check.
	{
		AdcTimer = 5; // Reset de timer (500 ms interval voor verversing van het display).

		// Als de groen knop lang (3 seconden) wordt ingedrukt willen we het systeem in kalibratie mode
		// zetten. Met onderstaande stukje code wordt conditie getest en de code hiervoor uitgevoerd.
		
		if(current_state == GREEN_PRESSED) // Als de groene knop ingedrukt is.
		{
			if(!calibration_mode)	// Als we nog niet in kalibratie-modus zitten.
			{
				if(pressed_time_counter++ >= 6) // Als de groene knop langer dan 3 seconden ingedrukt blijft (6 x 500 ms).
				{ 
          Serial.println("Enter calibration mode"); // Informeer via seriële monitor dat we in kalibratie-modus gaan
					display.setCursor(0,0); 									// Cursor op regel 1
					display.print("Calibration mode");				// Tekst voor regel 1
					display.setCursor(0,1);										// Cursor op regel 2
					display.print("1000 g   Nul- T+");				// Tekst voor regel 2
					calibration_weight = 1000;								// Stel het kalibratiegewicht in op 1000 g
					calibration_mode = true;									// Zet de kalibratie-modus aan.
				}
			}
		}

		// Als de gele knop lang (3 seconden) wordt ingedrukt willen we het systeem resetten.
		// Hieronder de code die de conditie hiervoor test en de reset-code uitvoert.

		if(current_state == YELLOW_PRESSED) 			// Als de gele knop ingedrukt is
		{
				if(pressed_time_counter++ >= 6) 			// Als de gele knop langer dan 3 seconden ingedrukt blijft (6 x 500 ms).
				{ 
					display.setCursor(0,0); 						// Cursor op regel 1
					display.print("Perfom a reset! ");	// Informeer de gebruiker via het display en de
          Serial.println("Perfom a reset!");	// seriële monitor dat we een reset gaan uitvoeren
				  delay(500);													// Voeg een halve seconde wachttijd in, deze is nodig, zodat
				  																		// we er op aan kunnen dat seriële tekst is verzonden voordat
				  																		// de reset in werking treed.
				  asm volatile ("  jmp 0");  					// Reset de Arduino met een assembler instructie
																							// assembler zijn low-level processor instructies
																							// vrij vertaal staat hier spring naar adres nul
				}
		}

		if(!calibration_mode) // Als het systeem in kalibratie mode staat
		{
			scale_found = scale.is_ready(); // Waarde is true als de HX711 ADC is aangetroffen
	
			if(scale_found) 
			{
				float display_weight;
				const char *gross_net_txt;
				long gross_int;  								// Voor bruto gewicht oplag als geheel getal in gram
				static long previous_gross_int;
				bool gross_changed;
					
				adc_sample = scale.read();		// Haal een weeg sample op uit de ADC
	
				// Dit is de basis berekening voor het uitrekenen van het bruto gewicht.
				// We nemen het actuele ADC-waarde en trekken eerst de ADC-waarde van een onbelaste weegschaal er af.
				// Het verschil vermenigvuldigen we met een factor die het aantal gram per ADC-waarde representeert.
				// Het resultaat hiervan is het gewichtswaarde van de aangebrachte ballast.
				
				// Dit is de basis formule voor en gewicht calculatie
				gross = (adc_sample - myConfig.zero_ad_value) * myConfig.weigh_factor;  

					// Bepaal het netto gewicht aan de hand van eerder opgeslagen tarra gewicht
				net = gross - tare;

				// Zet het decimale float getal voor het bruto gewicht om in een vast getal waarbij 
				// de als het ware alle decimalen worden geelimineerd. Enkel het bruto gewicht in gram 
				// blijft over. De notatie met (long) voor de van origine float variable noemen we 
				// casten. De float wordt op dat punt omgezet naar een unsigned long. Bij casten van
				// float naar long worden de decimalen afgekapt. Dit maakt dat er onjuist wordt afgerond.
				// Een truc hiervoor (dus zodat de afronding juist klopt) is door er eerst 0,5 bij op te letten.
								
				gross_int = (long) (gross + 0.5);		 // Decimaal getal omzetten naar geheel getal (met afronding)
				if(gross_int !=  previous_gross_int) // Als het bruto gewicht is veranderd, (dus in dit 
				{																		 // geval alleen het niet decimale deel in gram).
					gross_changed = true;
					previous_gross_int = gross_int;		 // Voorgaande waarde met nieuwe updaten voor volgende cyclus
				}
				else
				{
					gross_changed = false;
				}

					// Bepaal welk gewicht er op het display komt netto of bruto en de bijbehorende tekst
				display_weight = net_view ? net : gross;
				gross_net_txt = net_view ? net_txt : gross_txt;
	
					// Formateer de variable naar een string voor op het display
				dtostrf(display_weight, 7, 0, str_display_weight);

				// Bij deze stringformattering gebruiken we de %s-notatie in de sprintf() functie,
				// de eerste is voor de bruto tekst (gross_net_txt) en de tweede voor het display 
				// gewicht (str_display_weight). Voor meer info over de functie is mijn advies 
				// om ChatGPT te raadplegen, hier wordt de functie heel duidelijk uitgelegd.
					
				sprintf(display_line_two, "%-6.6s %s g", gross_net_txt, str_display_weight);
				display.setCursor(0,1);						// Zet cursor vooraan op de tweede regel
				display.print(display_line_two);  // en zet de  gewichtsregel op het display

				// Hier gebruiken we weer de string opmaak functie sprintf(), nu met
				// %7ld , de letter ld staan voor long(int) waarmee het type van de 
				// eerst opgegeven parameter wordt aangegeven, dus in dit geval van adc_sample

				if(gross_changed){ //Schrijf enkel naar serieele output als bruto gewicht (in gram) is veranderd
					sprintf(str_format_buffer, "AD-waarde: %7ld  => Gross: %6ld g", adc_sample, gross_int);
					Serial.println(str_format_buffer);		// Zet de regel naar de serieele output
				}
				
				//======================================
				//Zet hieronder de code voor opdracht 1 
				//======================================
				if(gross_int > 350) 									// Als het bruto gewicht groter is dan 350 kg 
				{
				  digitalWrite(LIGHT_GREEN_PIN,HIGH); // Zet de groene led uit
				  digitalWrite(LIGHT_RED_PIN,LOW);   	// Zet de rode led aan
				}
				else																	// Als het bruto gewicht kleiner (of gelijk) is dan 350 kg 
				{																			
				  digitalWrite(LIGHT_GREEN_PIN,LOW);  // Zet de groene led aan
				  digitalWrite(LIGHT_RED_PIN,HIGH);   // Zet de rode led uit
				}	
			}
			 else 
			{
				// Het programma komt hier als de ADC-converter niet is aangetroffen.
				// Als dit bij de vorige cyclus nog wel het geval was, wordt onderstaande 
				// tekst naar de seriële output gestuurd.

				if(prev_scale_found != scale_found)
				{
					Serial.println("Error: HX711 not found!");
				}
			}
	
			if(prev_scale_found != scale_found)
				prev_scale_found = scale_found;
		}
	}	

	// Vanuit de hoofdlus loop() moet de timer worden geserviced.
	// Dit doen we met onderstaande functie.
	timer.update(); 
	
} // Einde van de loop() functie

//-----------------------------------------------------------------------------------------
// Deel 9: Eigen functies

// Hieronder volgen twee zelfgemaakte functies. De eerste is een zogenaamde interrupt 
// functie, die wordt aangeroepen bij een bepaalde gebeurtenis, meestal vanuit de hardware. 
// Dit gebeurt wanneer er iets is voorgevallen waarop de software direct moet reageren. In dit geval 
// heeft het te maken met het verstrijken van een bepaalde tijd. Deze interrupt wordt geactiveerd 
// wanneer er 100 ms zijn verstreken (ten opzichte van de vorige aanroep).
// Dit betekent dat onderstaande functie iedere 100 ms wordt uitgevoerd. Wanneer een interrupt 
// zich voordoet, stopt het programma in de hoofdlus en wordt de interrupt direct afgehandeld. 
// Na het uitvoeren van de interrupt functie keert het systeem terug naar het punt waar het programma 
// was gestopt en gaat daar verder.

void timer_interrupt()
{
	// De globale variable AdcTimer wordt in deze functie verlaagt met één,
	// zolang deze nog niet nul is.
	
	if(AdcTimer) 
		AdcTimer--;
}

// In de onderstaande functie bepalen we welke drukknop is ingedrukt. We gaan er hierbij vanuit 
// dat er slechts één drukknop tegelijk wordt ingedrukt. In de praktijk hoeft dit echter niet altijd 
// het geval te zijn, maar omdat de software geen rekening houdt met situaties waarin meerdere 
// knoppen tegelijkertijd ingedrukt zijn, is dit voor deze functie niet relevant. 
// Als er wel meerdere knoppen tegelijk worden ingedrukt, zou er sprake kunnen zijn van een dominantie- 
// volgorde. Met de kennis die je tot nu toe hebt opgedaan, zou je in staat moeten zijn om te begrijpen 
// hoe deze volgorde kan worden bepaald.

ePUSH_BUTTON get_push_button()
{
		//Read out push buttons
	if(digitalRead(SWITCH_YELLOW_PIN) == LOW)		 	return YELLOW_PRESSED;
	else if(digitalRead(SWITCH_GREEN_PIN) == LOW) return GREEN_PRESSED;
	else if(digitalRead(SWITCH_BLUE_PIN) == LOW)	return BLUE_PRESSED;
	else																					return NONE_PRESSED;
}
