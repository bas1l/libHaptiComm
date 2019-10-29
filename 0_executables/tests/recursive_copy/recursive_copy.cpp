using bfs = boost::filesystem;

void recursive_copy(const bfs::path &src, const bfs::path &dst)
{
  if (bfs::exists(dst)){
    throw std::runtime_error(dst.generic_string() + " exists");
  }

  if (bfs::is_directory(src)) {
    bfs::create_directories(dst);
    for (bfs::directory_entry& item : bfs::directory_iterator(src)) {
      recursive_copy(item.path(), dst/item.path().filename());
    }
  } 
  else if (bfs::is_regular_file(src)) {
    bfs::copy(src, dst);
  } 
  else {
    throw std::runtime_error(dst.generic_string() + " not dir or file");
  }
}