import java.sql.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

/*
 Name: Jay K Roy
 CWID: 12342760
 Project: CS 301 Bonus - DB connect using java
 */

public class Bonus {
    // Database connection constants
    private static final String DB_URL = "jdbc:oracle:thin:@//oracle.cs.ua.edu:1521/xe";
    private static final String DB_USERNAME = "jroy4";
    private static final String DB_PASS = "12342760";
    
    public static void main(String[] args) {
        Connection dbConnection = null;
        Scanner inputScanner = new Scanner(System.in);
        String userInput;
        
        try {
            // Initialize Oracle JDBC driver
            Class.forName("oracle.jdbc.driver.OracleDriver");
            
            // Create database connection
            dbConnection = DriverManager.getConnection(DB_URL, DB_USERNAME, DB_PASS);
            
            // Application loop
            boolean running = true;
            while (running) {
                System.out.print("Enter a department number or a department name: ");
                userInput = inputScanner.nextLine().trim();
                
                // Exit condition check
                if (userInput.equals("0") || userInput.equalsIgnoreCase("quit")) {
                    System.out.println("\nGood Bye!");
                    running = false;
                    continue;
                }
                
                // Handle department lookup
                handleDepartmentLookup(dbConnection, userInput);
                System.out.println();
            }
            
        } catch (ClassNotFoundException ex) {
            System.out.println("Failed to load Oracle JDBC Driver");
            ex.printStackTrace();
        } catch (SQLException ex) {
            System.out.println("Database error occurred: " + ex.getMessage());
            ex.printStackTrace();
        } finally {
            // Cleanup resources
            try {
                if (dbConnection != null) dbConnection.close();
                inputScanner.close();
            } catch (SQLException ex) {
                System.out.println("Error while closing connection: " + ex.getMessage());
            }
        }
    }
    
    private static void handleDepartmentLookup(Connection dbConnection, String userInput) throws SQLException {
        // Query to find department
        String departmentQuery = "SELECT DNUMBER, DNAME, MGRSSN FROM DEPARTMENT WHERE DNUMBER = ? OR UPPER(DNAME) = UPPER(?)";
        
        try (PreparedStatement prepStmt = dbConnection.prepareStatement(departmentQuery)) {
            // Handle numeric input
            try {
                int deptId = Integer.parseInt(userInput);
                prepStmt.setInt(1, deptId);
            } catch (NumberFormatException ex) {
                prepStmt.setNull(1, Types.INTEGER);
            }
            
            prepStmt.setString(2, userInput);
            
            try (ResultSet resultSet = prepStmt.executeQuery()) {
                if (resultSet.next()) {
                    int departmentId = resultSet.getInt("DNUMBER");
                    String departmentName = resultSet.getString("DNAME");
                    String managerSsn = resultSet.getString("MGRSSN");
                    
                    System.out.println("\nInformation about department #" + departmentId + "-" + departmentName + ":");
                    
                    // Display department information
                    displayManagerInfo(dbConnection, managerSsn);
                    
                    // Count employees
                    int employeeCount = countDepartmentEmployees(dbConnection, departmentId);
                    System.out.println("Number of employees: " + employeeCount);
                    
                    // Count dependents
                    int dependentCount = countDepartmentDependents(dbConnection, departmentId);
                    System.out.println("Number of dependents: " + dependentCount);
                    
                    // Display project information
                    listDepartmentControlledProjects(dbConnection, departmentId);
                    listProjectsWithAtLeastOneEmployee(dbConnection, departmentId);
                    listProjectsWithAllEmployees(dbConnection, departmentId, employeeCount);
                    
                } else {
                    System.out.println("\nNo department found");
                }
            }
        }
    }
    
    private static void displayManagerInfo(Connection dbConnection, String managerSsn) throws SQLException {
        String query = "SELECT FNAME, MINIT, LNAME FROM EMPLOYEE WHERE SSN = ?";
        
        try (PreparedStatement prepStmt = dbConnection.prepareStatement(query)) {
            prepStmt.setString(1, managerSsn);
            
            try (ResultSet resultSet = prepStmt.executeQuery()) {
                if (resultSet.next()) {
                    String firstName = resultSet.getString("FNAME");
                    String middleInitial = resultSet.getString("MINIT");
                    String lastName = resultSet.getString("LNAME");
                    
                    System.out.println("Department manager: " + firstName + " " + middleInitial + ". " + lastName);
                } else {
                    System.out.println("Department manager: Not found");
                }
            }
        }
    }
    
    private static int countDepartmentEmployees(Connection dbConnection, int departmentId) throws SQLException {
        String query = "SELECT COUNT(*) AS EMPLOYEE_COUNT FROM EMPLOYEE WHERE DNO = ?";
        int count = 0;
        
        try (PreparedStatement prepStmt = dbConnection.prepareStatement(query)) {
            prepStmt.setInt(1, departmentId);
            
            try (ResultSet resultSet = prepStmt.executeQuery()) {
                if (resultSet.next()) {
                    count = resultSet.getInt("EMPLOYEE_COUNT");
                }
            }
        }
        
        return count;
    }
    
private static int countDepartmentDependents(Connection dbConnection, int departmentId) throws SQLException {
    String deptIdStr = String.valueOf(departmentId);
    
    // Use the exact same query structure as the working example
    String query = "SELECT COUNT(*) AS DEPENDENT_COUNT " +
                   "FROM DEPENDENT D " +
                   "JOIN EMPLOYEE E ON D.ESSN = E.SSN " +
                   "WHERE E.DNO = ?";
                
    int count = 0;
    
    try (PreparedStatement prepStmt = dbConnection.prepareStatement(query)) {
        // Use setString instead of setInt to match the working example
        prepStmt.setString(1, deptIdStr);
        
        try (ResultSet resultSet = prepStmt.executeQuery()) {
            if (resultSet.next()) {
                count = resultSet.getInt("DEPENDENT_COUNT");
            }
        }
    }
        
    return count;
}        
    private static void listDepartmentControlledProjects(Connection dbConnection, int departmentId) throws SQLException {
        String query = "SELECT PNUMBER, PNAME FROM PROJECT WHERE DNUM = ? ORDER BY PNUMBER";
        
        try (PreparedStatement prepStmt = dbConnection.prepareStatement(query)) {
            prepStmt.setInt(1, departmentId);
            
            try (ResultSet resultSet = prepStmt.executeQuery()) {
                System.out.println("Projects controlled by the department:");
                boolean projectFound = false;
                
                while (resultSet.next()) {
                    projectFound = true;
                    int projectNumber = resultSet.getInt("PNUMBER");
                    String projectName = resultSet.getString("PNAME");
                    System.out.printf("     %2d %-20s\n", projectNumber, projectName);
                }
                
                if (!projectFound) {
                    System.out.println("     None");
                }
            }
        }
    }
    
    private static void listProjectsWithAtLeastOneEmployee(Connection dbConnection, int departmentId) throws SQLException {
        String query = "SELECT DISTINCT P.PNUMBER, P.PNAME " +
                      "FROM PROJECT P, WORKS_ON W, EMPLOYEE E " +
                      "WHERE P.PNUMBER = W.PNO AND W.ESSN = E.SSN AND E.DNO = ? " +
                      "ORDER BY P.PNUMBER";
        
        try (PreparedStatement prepStmt = dbConnection.prepareStatement(query)) {
            prepStmt.setInt(1, departmentId);
            
            try (ResultSet resultSet = prepStmt.executeQuery()) {
                System.out.println("Projects worked on by at least one employee:");
                boolean projectFound = false;
                
                while (resultSet.next()) {
                    projectFound = true;
                    int projectNumber = resultSet.getInt("PNUMBER");
                    String projectName = resultSet.getString("PNAME");
                    System.out.printf("     %2d %-20s\n", projectNumber, projectName);
                }
                
                if (!projectFound) {
                    System.out.println("     None");
                }
            }
        }
    }
    
    private static void listProjectsWithAllEmployees(Connection dbConnection, int departmentId, int employeeCount) throws SQLException {
        if (employeeCount == 0) {
            System.out.println("Projects worked on by all the employees: None");
            return;
        }
        
        // Fetch all projects worked on by department employees
        List<ProjectData> departmentProjects = new ArrayList<>();
        String projectQuery = "SELECT DISTINCT P.PNUMBER, P.PNAME " +
                             "FROM PROJECT P, WORKS_ON W, EMPLOYEE E " +
                             "WHERE P.PNUMBER = W.PNO AND W.ESSN = E.SSN AND E.DNO = ?";
        
        try (PreparedStatement prepStmt = dbConnection.prepareStatement(projectQuery)) {
            prepStmt.setInt(1, departmentId);
            
            try (ResultSet resultSet = prepStmt.executeQuery()) {
                while (resultSet.next()) {
                    int projectNumber = resultSet.getInt("PNUMBER");
                    String projectName = resultSet.getString("PNAME");
                    departmentProjects.add(new ProjectData(projectNumber, projectName));
                }
            }
        }
        
        // Check which projects have all employees working on them
        List<ProjectData> universalProjects = new ArrayList<>();
        String countQuery = "SELECT COUNT(DISTINCT E.SSN) AS EMPLOYEE_COUNT " +
                           "FROM EMPLOYEE E, WORKS_ON W " +
                           "WHERE E.SSN = W.ESSN AND E.DNO = ? AND W.PNO = ?";
        
        try (PreparedStatement prepStmt = dbConnection.prepareStatement(countQuery)) {
            for (ProjectData project : departmentProjects) {
                prepStmt.setInt(1, departmentId);
                prepStmt.setInt(2, project.getProjectNumber());
                
                try (ResultSet resultSet = prepStmt.executeQuery()) {
                    if (resultSet.next()) {
                        int projectEmployeeCount = resultSet.getInt("EMPLOYEE_COUNT");
                        if (projectEmployeeCount == employeeCount) {
                            universalProjects.add(project);
                        }
                    }
                }
            }
        }
        
        // Display results
        System.out.println("Projects worked on by all the employees:");
        if (universalProjects.isEmpty()) {
            System.out.println("     None");
        } else {
            for (ProjectData project : universalProjects) {
                System.out.printf("     %2d %-20s\n", project.getProjectNumber(), project.getProjectName());
            }
        }
    }
    
    // Project data container class
    private static class ProjectData {
        private int projectNumber;
        private String projectName;
        
        public ProjectData(int projectNumber, String projectName) {
            this.projectNumber = projectNumber;
            this.projectName = projectName;
        }
        
        public int getProjectNumber() {
            return projectNumber;
        }
        
        public String getProjectName() {
            return projectName;
        }
    }
}