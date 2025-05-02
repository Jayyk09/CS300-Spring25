/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Other/File.java to edit this template
 */
// package cs301app;

/**
 *
 * @author RIDWANULLAH
 */
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.PreparedStatement;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;

public class FetchDataFromOracle {
    public static void main(String[] args) throws FileNotFoundException {
        // Define Oracle DB properties
        Scanner passwd_scnr = new Scanner(new File("userinfo.txt"));
        String username = passwd_scnr.nextLine();
        String password = passwd_scnr.nextLine();

        String jdbcUrl = "jdbc:oracle:thin:@oracle.cs.ua.edu:1521:xe";
        
        // Initialize the JDBC connection and statement
        try {
            Connection connection = DriverManager.getConnection(jdbcUrl, username, password);
            
            Scanner scanner = new Scanner(System.in);
            // Get Department Number from the user
            System.out.print("Enter a Department Number: ");
            
            String deptNumber = scanner.nextLine();
            
            // Check if the Department Number only contains numbers
            if (!deptNumber.matches("\\d+")) {
                System.out.println("Department Number should only contain numbers.");
                return;
            }

            // Create the SQL query to get all dependents for a department
            String sqlQuery = "SELECT D.DEPENDENT_NAME, D.RELATIONSHIP, E.FNAME, E.LNAME " +
                              "FROM DEPENDENT D " +
                              "JOIN EMPLOYEE E ON D.ESSN = E.SSN " +
                              "WHERE E.DNO = ?";
            
            try {
                PreparedStatement preparedStatement = connection.prepareStatement(sqlQuery);
                // Set the user input as a parameter in the prepared statement
                preparedStatement.setString(1, deptNumber);
                // Execute the query
                ResultSet resultSet = preparedStatement.executeQuery();
                
                // Process the ResultSet
                System.out.println("Dependents for Department " + deptNumber + ":");
                boolean hasDependents = false;
                while (resultSet.next()) {
                    hasDependents = true;
                    String dependentName = resultSet.getString("DEPENDENT_NAME");
                    String relationship = resultSet.getString("RELATIONSHIP");
                    String employeeFirstName = resultSet.getString("FNAME");
                    String employeeLastName = resultSet.getString("LNAME");
                    System.out.printf("Dependent Name: %s, Relationship: %s, Employee: %s %s\n", 
                                      dependentName, relationship, employeeFirstName, employeeLastName);
                }
                if (!hasDependents) {
                    System.out.println("No dependents found for this department.");
                }
            } catch (SQLException e) {
                e.printStackTrace();
            }
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }
}
