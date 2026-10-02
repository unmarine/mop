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

std::string Trim(std::string &source) {
  std::string buffer = "";
  int lower_bound = 0;
  int upper_bound = source.length();

  while (source[lower_bound] == ' ') lower_bound++;
  while (source[upper_bound] == ' ') upper_bound--;

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

std::vector<Token> Parse(std::string &source) {
  std::vector<Token> tokens;

  std::vector<std::string> statements = SplitByDelimeter(source, '.');

  std::regex from_pattern(R"(from\s*\"(.+)\"\s*text\s*\"(.+)\"\s*as\s*\"(.+)\")");
  std::regex root_pattern(R"(root\s*\"(.+)\"\s*as\s*\"(.+)\")");
  
  for (std::string& statement : statements) {
	std::smatch matches;
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
	else if (std::regex_search(statement, matches, root_pattern)) {
	  std::string content_of_root_node = matches[1].str();
	  std::string reference_of_root_node = matches[2].str();
	  tokens.push_back({
		  TokenType::RootNode,
		  "",
		  content_of_root_node,
		  reference_of_root_node
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

  double x;
  double y;
  
  Node(Node* parent, std::string content) {
	this->content = content;
	this->parent = parent;
  }
};

class ReferenceTable {
private:
  inline static std::map<std::string, Node*> table;
  
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

void Render(Node* root, const char* filename, const char* format = "png") {
  if (!root) return;

  GVC_t* gvc = gvContext();

  Agraph_t* g = agopen((char*)"mindmap", Agdirected, nullptr);

  agattr(g, AGRAPH, (char*)"rankdir", (char*)"TB");
  agattr(g, AGNODE, (char*)"fontsize", (char*)"14");


  agsafeset(g, (char*)"nodesep", (char*)"0.2", (char*)"");
  // agsafeset(g, (char*)"size", (char*)"6.4,3.6!", (char*)"");
  agsafeset(g, (char*)"dpi", (char*)"300", (char*)"");
  // agsafeset(g, (char*)"overlap", (char*)"scale", (char*)"");
  // agsafeset(g, (char*)"overlap", (char*)"false", (char*)"");
  agsafeset(g, (char*)"nodesep", (char*)"5.0", (char*)"");
  agsafeset(g, (char*)"ranksep", (char*)"4.3", (char*)"");
  
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

  gvLayout(gvc, g, "twopi");
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
  
  app.add_option("-i", filename, "Path of your .mop file.");
  app.add_option("-o", output_filename, "Path of your generated mindmap. Does not define the format, so write .png yourself.");
  app.add_option("-f", format, "Format of your file, e.g. png, svg, etc.");
  CLI11_PARSE(app, argc, argv);
  
  std::string source = ReadFile(filename);

  std::vector<Token> tokens = Parse(source);
  Interpret(tokens);
  Node* root = ReferenceTable::GetNode("root");
  Render(root, output_filename.c_str(), format.c_str());
}

