# persistent_text_editor
A simple text editor which stores versions of text files

# How to run?
On your terminal type ./editor \<filename\> \<version\>to open the required file. You can also simply type ./editor to generate a file.
You can also open the original file by not mentioning the version number or
simply using zero.

# Utils
Use 'CTRL+O' to open required file. Enter filename and version number.

Use 'CTRL+S' to save a file.
You can only make changes in the newest version of the file. Although you can
change the older ones, the versions next to the currently open version gets
modified and all the previous data is lost.

Use 'CTRL+Q' to quit from the editor.
Restores the terminal back to its original form.

