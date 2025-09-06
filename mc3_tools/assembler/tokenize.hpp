#include <string>
#include <vector>

std::vector<std::string> tokenize(const std::string& assembly)
{
  std::vector<std::string> tokens;

  for (std::size_t t = 0; t != -1; t = assembly.find_first_not_of(" \t\n", t))
  {
    if (assembly[t] == '@')
    {
      tokens.emplace_back("@");
      t++;
    } else if (assembly[t] == '+')
    {
      tokens.emplace_back("+");
      t++;
    } else if (assembly[t] == '[')
    {
      tokens.emplace_back("[");
      t++;
    } else if (assembly[t] == ']')
    {
      tokens.emplace_back("]");
      t++;
    } else if (assembly[t] == '-')
    {
      tokens.emplace_back("-");
      t++;
    } else if (assembly[t] >= '0' && assembly[t] <= '9')
    {
      std::size_t size = 0;
      tokens.emplace_back(std::to_string(std::stoi(assembly.substr(t), &size, 0)));
      t += size;
    } else
    {
      std::size_t pos = assembly.find_first_of(" \t\n+-[]", t);
      tokens.emplace_back(assembly.substr(t, pos - t));
      t = pos;
    }
  }

  return tokens;
}
