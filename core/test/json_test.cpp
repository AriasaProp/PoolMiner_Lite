#include "json.hpp"
#include <iostream>

bool json_test () {
	 
    std::string str = R"({
    animals : [
      { name: 'Cat', coordinates: [2, 5, 8], height: 1.5, comment: "It's saying...\t meow." },
      { 'name': Dog, coordinates: [-4, 3], height: 2.5, width: 0.8 }
      { name: Cow, coordinates: [-7, 0, -3], 'height': 4.0, comment: "It says... \t moo." },
      { name: "Goat", coordinates: [1, -8, -9, 4], comment: "It's saying...\t 'baaa..'" }
    ],
    owner : { name: 'Sam', age: 32, address: '123 Animal Drive, New York, 98765.' }
    })";
    std::cout << "The JSON string:\n\n" << str << "\n" << std::endl;
    
    RSJresource  my_resource (str); 
    // RSJ 
    std::cout << "\nDirect access:\n";
    std::cout << "\t The cow's X-coordinate is: " << my_resource["animals"][2]["coordinates"][0].as<int>() << std::endl;
    std::cout << "\t The cat's address is: " << my_resource["animals"][0]["address"].as<std::string>("unknown address") << std::endl;
    std::cout << "\t The owner's age is: " << my_resource["owner"]["age"].as<double>(50) << std::endl;
    RSJresource& dog_resource = my_resource["animals"][1]; // refernce
    std::cout << "\t The dog's Y-coordinate is: " << dog_resource["coordinates"][1].as<double>() << std::endl;
    
    // ----
    std::cout << "\nLooping through the animals:\n";
    for (auto it=my_resource["animals"].as_array().begin(); it!=my_resource["animals"].as_array().end(); ++it)
        std::cout << "\t The '" << (*it)["name"].as<std::string>() 
                        << "' has height " << (*it)["height"].as<double>(-1.0) << ". "
                        << ((*it)["width"].exists() ? "Its width = "+(*it)["width"].as<std::string>()+". " : "It has undefined width. ")
                        << "It lives in a " << (*it)["coordinates"].size() << " dimensional space. "
                        << (*it)["comment"].as<std::string>("(there is no comment on this animal's sound).")
                        << std::endl;
                        
    // ----
    std::cout << "\nInserting admirers:\n";
    my_resource["admirers"] = RSJresource(  "{  \n"
                                            "    'Jane': [cat, dog],  \n" 
                                            "    'Jonathan': {admires: ['cat', 'goat'], relation_to_owner: 'brother'},  \n"
                                            "    Katie: cow  \n"
                                            "}" );
    for (auto it=my_resource["admirers"].as_object().begin(); it!=my_resource["admirers"].as_object().end(); ++it) {
        std::cout << "\t " << it->first << " admires ";
        if (it->second.type()==RSJ_ARRAY)
            for (int a=0; a<it->second.size(); ++a)
                std::cout << it->second[a].as<std::string>() + ", ";
        else if (it->second.type()==RSJ_OBJECT)
            for (int a=0; a<it->second["admires"].size(); ++a)
                std::cout << it->second["admires"][a].as<std::string>() + ", ";
        else if (it->second.type()==RSJ_LEAF)
            std::cout << it->second.as<std::string>();
        std::cout << std::endl;
    }
    
    // ----
    std::cout << "\nChanging width of dog, inserting its weight, and inserting its Z-coordinate...\n";
    dog_resource["width"] = 2.1;
    dog_resource["coordinates"][2] = -4.7;
    dog_resource["weight"] = 11;
    std::cout << "Accessing non-existent 'age' of dog: " << dog_resource["age"].as<int>(-1) << ". size: " <<  dog_resource["age"].size() << std::endl;
    
    std::cout << "\nprinting the details about dog:\n" << my_resource["animals"][1].as_str() << std::endl;
    
    // ----
    std::cout << "\nFinal complete resource printed:\n" << my_resource.as_str() << std::endl;
    
    return true;
}

