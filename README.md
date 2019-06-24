# Provinssi2017
An art project made for Provinssi 2017 festival

# Liike-energiateos, Arduino
Teoksen ohjauslogiikka toteutettiin käyttämällä Arduino Uno:a.

## Kierrosnopeuden mittaaminen
Alun perin kierrosnopeutta oli tarkoitus mitata niin, että jokaisella pyörällä olisi oma slave-prosessorinsa joka lukisi hall-anturilta dataa ja laskisi kyseisen pyörän kierrosnopeutta sekä yksi master-prosessori joka keräisi kaiken tämän datan. Tällöin saataisiin yksilöllistä tietoa kunkin pyörän nopeudesta, sekä systeemistä pystyttäisiin tehdä mm. dynaamisesti skaalautuva jos esimerkiksi yksi tai useampi pyörä tippuisi linjoilta odottamattomasti (= slave ei vastaa masterin kutsuun).
Yksinkertaisuuden ja halpuuden takia päädyttiin paljon yksinkertaisempaan systeemiin, jossa on yksi keskus Arduino sekä jokaisella pyörällä hall-anturin perässä linedriver joka välittää hall-anturin signaalin 5V kanttipulssina Arduinon digitaalipinniin. Tämän järjestelmän heikkoutena mainittakoon että yksittäisten pyörien kierrosnopeuden määrittäminen olisi kohtuullisen hidas prosessi, koska jokaisen vastaanotetun pulssin lähettäjää ei voida identifioida suoraan, tällöin lasketaan vaan kokonaiskierrosnopeutta ja jaetaan se pyörien määrällä saaden kohtuullisen arvion keskinopeudesta.
Laskenta on toteutettu kaavalla
> totRpm = ((revs/2)/(float)sampleDur)*60.0;

Tällöin laskennan tarkkuus on suoraan verrannollinen sampleDur:in pituuteen. Tähän tarkoitukseen tarkkuus on täysin riittävä, vaikkakin pienillä kierroksilla virheen suuruus on huomattavaa. Suurilla kierroksilla virheen määrä on minimaalinen, kuitenkaan laite tällaisenaan ei ole mikään tarkkuusmittaväline.

Mittaustarkkuus: sampleDur	RPM

  1 sekunti	60

  2 sekuntia	30

  3 sekuntia	20

  10 sekuntia	6

## Toteutustapa
Ohjelmallisesti kierrosnopeuden mittaaminen toteutettiin käyttämällä pin change interruptia sen nopeuden vuoksi, ATmega328P:stä löytyy mahdollisuus PCI:lle jokaisesta digitaalipinnistä sekä analogipinnistä, tosin D13 ei toiminut syystä tai toisesta. Arduinon oma LED on kytketty myös pinniin D3, joten tämä saattoi vaikuttaa asiaan. 
PCI:n tuoma haitta on sen laukeaminen sekä nousevalla että laskevalla pulssilla, toisin kuin external interrupt pinnit jotka voidaan konfiguroida laukeamaan pulssin laskevalla tai nousevalla laidalla. Liike-energiateoksessa puhutaan kuitenkin niin hitaista nopeuksista, että tämä ei muodostunut ongelmaksi. Nykyisellään Arduino pystyy laskemaan kierrosnopeuden maksimissaan 1.1 kHz taajuudelle ja tällöinkin pullonkaulaksi osoittautui u_16 kiepsahtaminen ympäri. Kokeellisesti havaittuna PCI:n maksimitaajuus on 120 kHz, tällöin tarkkuus pysyi vielä 0.56% epätarkkuuden alla, eli yksinkertaista +1 operaatiota Arduino pystyi suorittamaan 240 kHz taajuudella.

## Asetusten vaihto sarjayhteyden yli
Asiat eivät kuitenkaan mene niin kuin suunniteltua, joten ohjauslogiikan parametrejä täytyy voida muuttaa laitteen päälläollessa, tässä apuun tuli bluetooth-dongle (HC-05 tai HC-06 pienillä modauksilla) jonka tarkka malli jäi hieman mysteeriksi ja sarjayhteys. Arduinossa on valmiiksi erittäin helposti toimiva mahdollisuus sarjayhteyteen, joten tähän bluetoothin liittäminen oli lähinnä TX/RX pinnien kytkeminen oikein päin (eli ristiin). 

## Virheentunnistus
Koska ohjelman parametreja on mahdollista muuttaa bluetooth yhteyden kautta, ohjelmaan toteutettiin yksinkertainen mutta toimiva virheentunnistus. Komennoille on määritelty tietyt rajat joilla ohjelmaa ei saada sotkettua, jos ohjelma jostain syystä kuitenkin menisi täysin jumiin, 8 sekunnin kuluttua watchdog aktivoituu ja lataa muistiinsa turvalliset vakioiden arvot sekä käynnistyy uudelleen. Käyttäjää on ohjeistettu päättämään kaikki komentonsa merkkeihin ’\r\n’ jolloin ohjelma tietää suoraan komennon päättyneen, näiden merkkien puuttuessa ohjelma kuulostelee tuleeko lisäohjeita ja sekunnin hiljaiselon jälkeen parsii vastaanotetun komentorivin normaaliin tapaan. 

## EEPROM tallennus
Jotta halutut parametrit voidaan säilyttää odottamattoman tai odotetun uudelleenkäynnistyksen yli, päätettiin parametreja tallentaa Arduinon EEPROM-muistiin. Ohjelma hakee tallennetut arvot muististaan jokaisen käynnistyksen yhteydessä ja jos niissä havaitaan jotain outoa, palauttaa se ne oletusasetuksille.

## Interruptit
Kahdenlaisten kuntopyörien takia jouduttiin käyttämään kaikkia kolmea ATmega328P:n PCI-vektoreita. Kuntopyörät joissa kierrosnopeus luettiin vauhtipyörästä käyttivät analogipinnien PCI-vektoria PCINT2 kun taas kuntopyörät joissa sensori on polkimessa käyttävät digitaalipinnien PCI-vektoreita PCINT1 ja PCINT2.
Projektin Arduino käyttää sekä watchdog ajastinta (WDT) että Timer1 ajastinta, näistä WDT pitää huolen Arduinon uudelleenkäynnistämisestä jos se menee täysin jumiin ja Timer1 kierrättää automaattista tilannepäivitystä minuutin välein.
Molemmat ajastimet alustettiin Atmelin datasheetin mukaisesti, WDT ajaa oman interruptinsa ja käynnistää Arduinon uudelleen 8 sekunnin kuluttua jos ohjelma jää jostain syystä jumiin, Timer1 ajaa oman interruptinsa minuutin välein. 
