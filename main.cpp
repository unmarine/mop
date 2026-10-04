#include "CLI/CLI.hpp"

#include <graphviz/gvc.h>
#include <fstream>
#include <regex>
#include <vector>
#include <iostream>
#include <map>
#include <cmath>

enum TokenType {
  FromNode,
  RootNode
};

struct Token {
  TokenType type;
  std::string parent_reference;
  std::string content;
  std::string reference;
};

// void Render(Node* root, const char* filename, const char* dpi = "\0", const char* inch_width = "\0", const char* inch_height = "\0", const char* format = "png", const char* engine = "twopi") {
struct Config {
  std::string filename;
  std::string inch_width;
  std::string inch_height;
  std::string dpi;
  std::string format;
  std::string engine;
};

std::string Trim(std::string &source) {
  std::string buffer = "";
  int lower_bound = 0;
  int upper_bound = source.length();

  while (source[lower_bound] == ' ' || source[lower_bound] == '\t') lower_bound++;
  while (source[upper_bound] == ' ' || source[upper_bound] == '\t') upper_bound--;

  for (int i = lower_bound; i < upper_bound; i++) buffer.push_back(source[i]);
  return buffer;
}

std::vector<std::string> SplitByDelimeter(std::string &source, char delimeter) {
  std::vector<std::string> strings;

  std::string buffer = "";
  size_t position = 0;
  while (position < source.length()) {
	if (source[position] == delimeter) {
	  strings.push_back(Trim(buffer));
	  buffer.clear();
	} else {
	  buffer.push_back(source[position]);
	}
	position++;
  }

  std::string trimmed = Trim(buffer);
  if (!trimmed.empty()) strings.push_back(trimmed);
  return strings;
}


/*
  from "phonetics" texts [
  "Long and short vowels" as "landsv",
  "Matres lectionis" as "matlec",
  "Full and defective spellings" as "fanddspel",
  "Shewa" as "shewa"
  ].
*/
/*
std::smatch MatchTextReference(std::string &source) {
  std::regex assignation_pattern(R"(\"(.*)\"\s*as\s*\"(.*)\")");
  
  std::smatch matches;
  // ugly syntax, maybe can simplify.
  if (std::regex_search(source, matches, assignation_pattern)) {
  } else {
	std::cout << "Failed to read text and reference in the string: " << source << std::endl;
  }
  return matches;
}

std::smatch MatchSingularAssignation(std::string &source) {
  st
}
*/

std::vector<Token> Parse(std::string &source) {
  std::vector<Token> tokens;

  std::vector<std::string> statements = SplitByDelimeter(source, '.');

  std::regex from_pattern(R"(from\s*\"(.+)\"\s*text\s*\"(.+)\"\s*as\s*\"(.*)\")");
  std::regex root_pattern(R"(root\s*\"(.+)\")");

  std::regex from_multiple_pattern(R"(from\s*\"(.+)\"\s*texts\s*\[(.*)\])");
  std::regex from_multiple_child_pattern(R"(\"(.+)\"\s*as\s*\"(.*)\")");
  
  for (std::string& statement : statements) {
	std::smatch matches;
	std::smatch child_matches;
	
	if (std::regex_search(statement, matches, from_pattern)) {
	  std::string reference_of_parent_node = matches[1].str();
	  std::string content_of_child_node = matches[2].str();
	  std::string reference_of_child_node = matches[3].str();
	  tokens.push_back({
		  TokenType::FromNode,
		  reference_of_parent_node,
		  content_of_child_node,
		  reference_of_child_node
		});
	}
	else if (std::regex_search(statement, matches, from_multiple_pattern)) {
	  std::string reference_of_parent_node = matches[1].str();
	  std::string children_descriptions = matches[2].str();

	  
	  
	  std::vector<std::string> descriptions = SplitByDelimeter(children_descriptions, ',');
	  for (std::string& description : descriptions) {
		if (std::regex_search(description, child_matches, from_multiple_child_pattern)) {
		  std::string content = child_matches[1];
		  std::string child_reference = child_matches[2];
		  tokens.push_back({
			  TokenType::FromNode,
			  reference_of_parent_node,
			  content,
			  child_reference
			});
		} else {
		  std::cout << "Invalid statement\n";
		}
	  }
	}
	  
	else if (std::regex_search(statement, matches, root_pattern)) {
	  std::string content_of_root_node = matches[1].str();
	  tokens.push_back({
		  TokenType::RootNode,
		  "",
		  content_of_root_node,
		  "root"
		});
	}
	else std::cout << "Invalid statment\n";
		
	matches = std::smatch();
  }
  return tokens;
}

class Node {
public:
  std::string content;
  std::vector<Node*> children;
  Node* parent;
  
  Node(Node* parent, std::string content) {
	this->content = content;
	this->parent = parent;
  }
};

class ReferenceTable {
private:
  inline static std::map<std::string, Node*> table;
  int unknown_reference_counter = 1; //
  std::string unknown_reference_prefix = "unknown_54209348092384023423339834989_";
  
public:
  static void AddReference(std::string& reference, Node* node) {
	table.insert({reference, node});
  }

  static bool CheckEmptyReference(std::string& reference) {
	auto it = table.find(reference);
	return it == table.end();
  }

  static Node* GetNode(const std::string& reference) {
	auto it = table.find(reference);
	if (it != table.end()) return it->second;
	return nullptr;
  }
};

void Interpret(std::vector<Token> tokens) {
  Node* root = nullptr;
  for (Token token: tokens) {
	if (token.type == TokenType::RootNode) {
	  root = new Node(nullptr, token.content);
	  ReferenceTable::AddReference(token.reference, root);
	  break;
	}
  }
  
  if (!root) {
	std::cout << "Failed to find root node." << "\n";
	return;
  }
  
  for (Token token: tokens) {
	if (token.type == TokenType::RootNode) continue;

	if (ReferenceTable::CheckEmptyReference(token.parent_reference)) {
	  std::cout << "Parent reference \"" << token.parent_reference << "\" was not found." << "\n"; 
	} else {
	  Node* parent = ReferenceTable::GetNode(token.parent_reference);
	  Node* child = new Node(parent, token.content);
	  parent->children.push_back(child);
	  ReferenceTable::AddReference(token.reference, child);

	}
  }
}

std::string ReadFile(std::string filename) {
  std::string buffer;
  std::ifstream file(filename);
  if (!file.is_open()) {
	std::cerr << "Unable to open the file" << "\n";
	return "";
  }

  std::string line;
  while (getline(file, line)) {
	buffer += line;
  }
  file.close();
  return buffer;
}

void ShowTokens(std::vector<Token> tokens) {
  for (Token token: tokens) {
	if (token.type == TokenType::RootNode) {
	  std::cout << "Root content: <<" << token.content << ">> | root reference: <<" << token.reference << ">>\n";
	} else {
	  std::cout << "Parent reference: <<" << token.parent_reference << " >>| content: <<" << token.content << " >> | reference: <<" << token.reference << ">>\n";
	}
  }
}

void Render(Node* root, const char* filename, const char* dpi = "\0", const char* inch_width = "\0", const char* inch_height = "\0", const char* format = "png", const char* engine = "twopi") {
  if (!root) return;

  GVC_t* gvc = gvContext();

  Agraph_t* g = agopen((char*)"mindmap", Agdirected, nullptr);
  agattr(g, AGRAPH, (char*)"ranksep", (char*)"1.0");
  agattr(g, AGRAPH, (char*)"overlap", (char*)"scalexy");
  agattr(g, AGRAPH, (char*)"splines", (char*)"true");

  if (dpi[0] != '\0') {
	agattr(g, AGRAPH, (char*)"dpi", dpi);
  }
  
  if (inch_width[0] != '\0' && inch_height[0] != '\0') {
	std::string size = std::string(inch_width) + "," + inch_height + "!";
	agattr(g, AGRAPH, (char*)"size", size.c_str());
  }
  agattr(g, AGRAPH, (char*)"ratio", (char*)"fill");
  

  
  agattr(g, AGNODE, (char*)"margin", (char*)"0.05,0.02");
  agattr(g, AGEDGE, (char*)"len", (char*)"1.0");
  agattr(g, AGRAPH, (char*)"sep", (char*)"+10");
  agattr(g, AGRAPH, (char*)"rankdir", (char*)"TB");
  agattr(g, AGNODE, (char*)"fontsize", (char*)"14");

  std::map<Node*, Agnode_t*> ag_nodes;

  std::vector<Node*> queue = { root };
  size_t index = 0;

  while (index < queue.size()) {
	Node* current = queue[index++];

	if (ag_nodes.find(current) == ag_nodes.end()) {
	  std::string node_id = "node_" + std::to_string(reinterpret_cast<uintptr_t>(current));
	  Agnode_t* ag_node = agnode(g, (char*)node_id.c_str(), 1);
	  agsafeset(ag_node, (char*)"label", (char*)current->content.c_str(), (char*)"");

	  if (current == root) {
		agsafeset(ag_node, (char*)"root", (char*)"true", (char*)"");
	  }

	  ag_nodes[current] = ag_node;
	}
	
	Agnode_t* parent_ag_node = ag_nodes[current];

	for (Node* child: current->children) {
	  std::string child_id = "node_" + std::to_string(reinterpret_cast<uintptr_t>(child));
	  Agnode_t* child_ag_node = agnode(g, (char*)child_id.c_str(), 1);
	  agsafeset(child_ag_node, (char*)"label", (char*)child->content.c_str(), (char*)"");
	  ag_nodes[child] = child_ag_node;
	  
	  queue.push_back(child);
	  agedge(g, parent_ag_node, child_ag_node, nullptr, 1);


	}
  }

  gvLayout(gvc, g, engine);
  gvRenderFilename(gvc, g, format, filename);

  gvFreeLayout(gvc, g);
  agclose(g);
  gvFreeContext(gvc);
}


int main(int argc, char** argv) {
  CLI::App app{"Mindmap output"};

  
  std::string filename = "";
  std::string output_filename = "";
  std::string format = "png";
  std::string engine = "twopi";
  std::string inch_width = "\0";
  std::string inch_height = "\0";
  std::string dpi = "\0";
  
  app.add_option("-i", filename, "Path of your .mop file.");
  app.add_option("-o", output_filename, "Path of your generated mindmap. Does not define the format, so write .png yourself.");
  app.add_option("-f", format, "Format of your file, e.g. png, svg, etc.");
  app.add_option("-e", engine, "Engine used to display your mindmap.");
  app.add_option("--dpi", dpi, "DPI of your image. Significantly increases amount of pixels.");
  app.add_option("--wd", inch_width, "Width of your image in inches.");
  app.add_option("--hg", inch_height, "Height of your image in inches.");
  
  CLI11_PARSE(app, argc, argv);
  
  if (filename == "" || output_filename == "") {
	std::cout << app.help() << std::endl;
	return 1;
  }
  
  std::string source = ReadFile(filename);

  std::vector<Token> tokens = Parse(source);
  Interpret(tokens);
  Node* root = ReferenceTable::GetNode("root");
  Render(root, output_filename.c_str(), dpi.c_str(), inch_width.c_str(), inch_height.c_str(), format.c_str(), engine.c_str());
}

// void Render(Node* root, const char* filename, const char* dpi = "\0", const char* inch_width = "\0", const char* inch_height = "\0", const char* format = "png", const char* engine = "twopi")
