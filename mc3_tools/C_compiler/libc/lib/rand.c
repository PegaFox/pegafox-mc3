
extern unsigned int _randomSeed;
int rand()
{
  return _randomSeed++;
}
