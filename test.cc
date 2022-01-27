#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <map>
#include <iostream>

using std::cout;
using std::endl;

int main()
{
  std::map<int, int> map;
  for (int i = 10; i <= 80; i += 10)
  {
    map[i] = i * 10;
  }
  
  cout << map.lower_bound(5)->first << endl;
  cout << map.lower_bound(9)->first << endl;
  cout << map.lower_bound(10)->first << endl;
  cout << map.lower_bound(11)->first << endl;
  cout << map.lower_bound(90)->first << endl;

  std::map<int, int>::iterator it = map.lower_bound(90);
  cout << (it == map.end()) << endl;
  cout << map.end()->first << endl;
  cout << (--it)->first << endl;

  return 0;
}