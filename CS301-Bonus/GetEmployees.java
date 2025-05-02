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

public class GetEmployees {

    /**
     * @param args the command line arguments
     */
    public static void main(String args[]) throws FileNotFoundException {
        // Define Oracle DB properties
	Scanner passwd_scnr = new Scanner(new File("userinfo.txt"));
        String username = passwd_scnr.nextLine();
        String password = passwd_scnr.nextLine();
	// System.out.println("Username: " + username);
	// System.out.println("Password: " + password);

        String jdbcUrl = "jdbc:oracle:thin:@oracle.cs.ua.edu:1521:xe";
    
        // Initialize the JDBC connection and statement
        try {
            Connection connection = DriverManager.getConnection(jdbcUrl, username, password);
            Scanner scanner = new Scanner(System.in);
            // Get Department Name from the user
            System.out.print("Enter the Department Name: ");
            
            String dname = scanner.nextLine();
            

            // Create the SQL query to get employee details from the department name
            String sqlQuery = "SELECT e.Ssn, e.Fname, e.Minit, e.Lname, e.Salary FROM DEPARTMENT JOIN EMPLOYEE e ON DEPARTMENT.DNUMBER = e.DNO WHERE DNAME = ?";
            
            try {
                PreparedStatement preparedStatement = connection.prepareStatement(sqlQuery); 
                // Set the user input as a parameter in the prepared statement
                preparedStatement.setString(1, dname);
                // Execute the query
                ResultSet resultSet = preparedStatement.executeQuery();
                // Process the ResultSet
                
                // checking if ResultSet is empty 
                if (!resultSet.isBeforeFirst() ) {    
                    System.out.println("The department has no employees."); 
                } 
                while (resultSet.next()) {
                    int ssnumber = resultSet.getInt("SSN");
                    String Fname = resultSet.getString("FNAME");
                    String Lname = resultSet.getString("LNAME");
                    String Minit = resultSet.getString("Minit");
                    String Salary = resultSet.getString("Salary");
                    // Display outputs
                    System.out.println("SSN: " + ssnumber + ", First Name: " + Fname +  ", Initial: " + Minit +
                           ", Last Name: " + Lname + ", Salary: " + Salary);
                }
            } catch (SQLException e) {
                e.printStackTrace();
            }
        } catch (SQLException e) {
            e.printStackTrace();
        }

    }
    
}
