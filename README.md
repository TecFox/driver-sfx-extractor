# Driver SFX Extractor #
A tool which extracts all sounds from the BLK files used in Driver 1 (PSX) &amp; Driver 2

## Download ##
The program can be downloaded from [here](https://www.dropbox.com/s/7z610ic5ieqc6qp/Driver%20SFX%20Extractor.zip?dl=0).

## Usage / Info ##
The tool expects a single argument in the form of a filename. Run it via the command line or just drag & drop the BLK file onto the exe. The program will create a directory with the same name and on the same level as the BLK file containing sub-folders for the sound banks which contain the converted individual sounds as WAV. If present the extracted sounds will also contain looping information. There are no names for the sound banks or the sounds so everything is numbered from zero counting upwards.

Please note: Don't run this program with any file other than VOICES.BLK and VOICES2.BLK. It WILL crash (Duh).  
Since these files have no header it's impossible to identify them so the program will blindly assume that you're giving it a valid one.
