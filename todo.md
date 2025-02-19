use the socket error function / improve error handling
refactor some function names to make obvious what is going on (where locks are), etc
make receiving close chat not instantly kill the chat on the remote side
find out why closing the chat is not removing/killing it on the remote side (now either side)
rename some variables like remaining_len


timeouts + polling
gui refreshing with thread conditions??
gui resizing

stop error msg from blanking screen
stop incoming msg from moving cursor
segfault on receiving close???? Not consistent
remove the window locks??
allow the server thread to shutdown cleaner
myname no space is accepted?

add display for own name
color warnings/errors
names with spaces??
