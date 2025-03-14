void removeComments(std::string& assembly)
{
  std::size_t pos = 0;
  while ((pos = assembly.find(">", pos)) != std::string::npos)
  {
    assembly.erase(pos, assembly.find("<", pos)-pos);
  }

  pos = 0;
  while ((pos = assembly.find("~", pos)) != std::string::npos)
  {
    assembly.erase(pos, assembly.find("\n", pos)-pos);
  }
}

std::string preprocess(std::string assembly)
{
  removeComments(assembly);

  return assembly;
}